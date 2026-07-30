/**
 * @file    app_tracking.c
 * @brief   八路循迹差速控制任务。
 * @author  weixin
 */
#include "app_tracking.h"

#include "app_motor.h"
#include "app_serial_monitor.h"
#include "board_tracking.h"
#include "result.h"
#include "tracking_inputs.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 循迹输入和目标速度的更新周期，单位为毫秒。 */
#define TRACKING_CONTROL_PERIOD_MS UINT32_C(5)
/* 设为1时周期打印循迹输入和目标速度，量产运行默认关闭。 */
#define TRACKING_STATUS_PRINT_ENABLE INT32_C(0)
#if (TRACKING_STATUS_PRINT_ENABLE == 1)
/* 循迹状态串口打印周期，单位为毫秒。 */
#define TRACKING_PRINT_PERIOD_MS UINT32_C(200)
#endif
/* 每个位置误差单位对应的轮速差，单位为编码器计数每秒。 */
#define TRACKING_DIFFERENTIAL_GAIN INT32_C(4000)
/* 误差滤波采用 Q8 定点数，避免循迹周期内执行浮点运算。 */
#define TRACKING_ERROR_FILTER_SCALE INT32_C(256)
/* 一阶低通每周期吸收三十二分之一的新误差，滤波系数为 0.03125。 */
#define TRACKING_ERROR_FILTER_DIVISOR INT32_C(64)
/* 滤波误差处于正负此值以内时取消差速，保持直线目标。 */
#define TRACKING_ERROR_DEADBAND INT32_C(2)
/* 循迹任务允许发布的单轮目标速度绝对值。 */
#define TRACKING_TARGET_SPEED_LIMIT INT32_C(200000)
/* 八路持续全白后停车的真实时间，单位为毫秒。 */
#define TRACKING_LOST_STOP_MS UINT32_C(1000)

/* io0 至 io7 从左到右的位置权重，负值表示线位于车体左侧。 */
static const int8_t s_tracking_weights[TRACKING_INPUTS_COUNT] = {
  -8, -6, -4, -3, 3, 4, 6, 8
};

/** @brief 循迹控制器跨采样周期保留的丢线状态。 */
struct tracking_control_state {
  int32_t filtered_error;   /* Q8 格式的一阶低通滤波误差。 */
  int32_t last_raw_error;   /* 最近一次检测到黑线时的原始权重误差。 */
  TickType_t lost_start;    /* 本次连续全白开始时的 RTOS Tick。 */
  bool line_seen;           /* 启动后是否至少检测到过一次黑线。 */
  bool loss_timing;         /* 当前是否正在累计连续全白时间。 */
};

/** @brief 一次循迹计算生成的左右轮目标速度。 */
struct tracking_wheel_targets {
  int32_t left_speed;       /* 左轮目标速度，单位为计数每秒。 */
  int32_t right_speed;      /* 右轮目标速度，单位为计数每秒。 */
};

/** @brief 本周期从串口配置读取的左右轮基础速度。 */
struct tracking_base_speeds {
  int32_t left_speed;       /* 左轮基础速度，单位为计数每秒。 */
  int32_t right_speed;      /* 右轮基础速度，单位为计数每秒。 */
};

/**
 * @brief  将循迹目标限制在电机目标邮箱允许的范围内。
 * @param  speed 待限制的轮速，单位为编码器计数每秒。
 * @return 限制后的轮速。
 */
static int32_t tracking_speed_limit(int32_t speed)
{
  if (speed > TRACKING_TARGET_SPEED_LIMIT) {
    return TRACKING_TARGET_SPEED_LIMIT;
  }
  if (speed < -TRACKING_TARGET_SPEED_LIMIT) {
    return -TRACKING_TARGET_SPEED_LIMIT;
  }
  return speed;
}

/**
 * @brief  对本周期位置误差执行 Q8 定点一阶低通滤波。
 * @param  control 循迹控制器的跨周期状态。
 * @param  measured_error 本周期计算出的原始位置误差。
 * @return 无返回值。
 * @note   首次检测到黑线时直接初始化，避免车辆起步响应延迟。
 */
static void tracking_error_filter_update(
    struct tracking_control_state *control,
    int32_t measured_error)
{
  int32_t target_error;
  int32_t error_delta;

  target_error = measured_error * TRACKING_ERROR_FILTER_SCALE;
  if (!control->line_seen) {
    control->filtered_error = target_error;
    return;
  }

  error_delta = target_error - control->filtered_error;
  control->filtered_error +=
      error_delta / TRACKING_ERROR_FILTER_DIVISOR;
}

/**
 * @brief  根据八路黑线输入计算加权位置误差。
 * @param  inputs 八路原始输入，低电平表示检测到黑线。
 * @param  error 用于接收位置误差，范围为 -15 至 15。
 * @return true 表示检测到黑线，false 表示八路均为白色。
 * @note   八路全黑按路口处理，原始误差置零后经滤波逐步回正。
 */
static bool tracking_error_calculate(
    const struct tracking_inputs_state *inputs,
    int32_t *error)
{
  int32_t weight_sum = 0;
  uint8_t black_count = UINT8_C(0);
  uint8_t index;

  for (index = UINT8_C(0);
       index < TRACKING_INPUTS_COUNT; index++) {
    if (!inputs->high[index]) {
      weight_sum += s_tracking_weights[index];
      black_count++;
    }
  }
  if (black_count == UINT8_C(0)) {
    return false;
  }

  *error = (black_count == TRACKING_INPUTS_COUNT) ?
           INT32_C(0) : weight_sum / (int32_t)black_count;
  return true;
}

/**
 * @brief  按当前位置和丢线状态生成左右轮目标速度。
 * @param  control 循迹控制器的跨周期状态。
 * @param  line_found 本周期是否检测到黑线。
 * @param  measured_error 本周期检测到的位置误差。
 * @param  base_speeds 本周期使用的左右轮基础速度。
 * @param  targets 用于接收左右轮目标速度。
 * @return 无返回值。
 */
static void tracking_targets_update(
    struct tracking_control_state *control,
    bool line_found,
    int32_t measured_error,
    const struct tracking_base_speeds *base_speeds,
    struct tracking_wheel_targets *targets)
{
  TickType_t current_time;
  TickType_t lost_time;
  int32_t deadband_q8;
  int32_t active_error;
  int32_t differential;

  current_time = xTaskGetTickCount();
  if (line_found) {
    control->last_raw_error = measured_error;
    tracking_error_filter_update(control, measured_error);
    control->line_seen = true;
    control->loss_timing = false;
  } else {
    if (control->line_seen) {
      tracking_error_filter_update(control,
                                   control->last_raw_error);
    }
    if (!control->loss_timing) {
      control->lost_start = current_time;
      control->loss_timing = true;
    }
  }

  lost_time = current_time - control->lost_start;
  if ((!control->line_seen) ||
      (control->loss_timing &&
       (lost_time >= pdMS_TO_TICKS(TRACKING_LOST_STOP_MS)))) {
    targets->left_speed = INT32_C(0);
    targets->right_speed = INT32_C(0);
    return;
  }

  deadband_q8 =
      TRACKING_ERROR_DEADBAND * TRACKING_ERROR_FILTER_SCALE;
  if (control->filtered_error < -deadband_q8) {
    active_error = control->filtered_error + deadband_q8;
  } else if (control->filtered_error > deadband_q8) {
    active_error = control->filtered_error - deadband_q8;
  } else {
    active_error = INT32_C(0);
  }
  differential = (active_error * TRACKING_DIFFERENTIAL_GAIN) /
                 TRACKING_ERROR_FILTER_SCALE;
  targets->left_speed = tracking_speed_limit(
      base_speeds->left_speed + differential);
  targets->right_speed = tracking_speed_limit(
      base_speeds->right_speed - differential);
}

/**
 * @brief  低频打印八路输入以及当前循迹控制结果。
 * @param  inputs 八路循迹输入快照。
 * @param  filtered_error 当前采用的 Q8 格式滤波误差。
 * @param  targets 当前发布的左右轮目标速度。
 * @return 无返回值。
 */
#if (TRACKING_STATUS_PRINT_ENABLE == 1)
static void tracking_status_print(
    const struct tracking_inputs_state *inputs,
    int32_t filtered_error,
    const struct tracking_wheel_targets *targets)
{
  printf("[tracking] %u%u%u%u%u%u%u%u mask=0x%02X "
         "error_q8=%ld target=%ld,%ld\r\n",
         (unsigned int)inputs->high[0],
         (unsigned int)inputs->high[1],
         (unsigned int)inputs->high[2],
         (unsigned int)inputs->high[3],
         (unsigned int)inputs->high[4],
         (unsigned int)inputs->high[5],
         (unsigned int)inputs->high[6],
         (unsigned int)inputs->high[7],
         (unsigned int)inputs->high_mask,
         (long)filtered_error,
         (long)targets->left_speed,
         (long)targets->right_speed);
}
#endif

/**
 * @brief  周期采样八路循迹输入并发布左右轮目标速度。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务不访问电机硬件，电机任务在下一闭环周期消费目标快照。
 */
void tracking_task(void *argument)
{
  struct tracking_control_state control = {0};
  struct tracking_inputs_state inputs_state;
  struct tracking_wheel_targets targets;
  struct tracking_base_speeds base_speeds;
  struct tracking_inputs *inputs;
#if (TRACKING_STATUS_PRINT_ENABLE == 1)
  TickType_t last_print_time;
#endif
  TickType_t last_wake_time;
#if (TRACKING_STATUS_PRINT_ENABLE == 1)
  TickType_t current_time;
#endif
  int32_t measured_error = INT32_C(0);
  bool line_found;
  int result;

  (void)argument;
  inputs = board_tracking_get();
  if (inputs == NULL) {
    printf("[tracking] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
#if (TRACKING_STATUS_PRINT_ENABLE == 1)
  last_print_time = last_wake_time;
#endif
  for (;;) {
    result = serial_monitor_drive_base_speeds_get(
        &base_speeds.left_speed, &base_speeds.right_speed);
    if (result == PLATFORM_OK) {
      result = tracking_inputs_sample(inputs, &inputs_state);
    }
    if (result == PLATFORM_OK) {
      line_found = tracking_error_calculate(&inputs_state,
                                             &measured_error);
      tracking_targets_update(&control, line_found,
                              measured_error, &base_speeds,
                              &targets);
      result = motor_target_speeds_set(targets.left_speed,
                                       targets.right_speed);
    }
    if (result != PLATFORM_OK) {
      (void)motor_target_speeds_set(INT32_C(0), INT32_C(0));
      printf("[tracking] control failed: %d\r\n", result);
      vTaskDelete(NULL);
      return;
    }

#if (TRACKING_STATUS_PRINT_ENABLE == 1)
    current_time = xTaskGetTickCount();
    if ((TickType_t)(current_time - last_print_time) >=
        pdMS_TO_TICKS(TRACKING_PRINT_PERIOD_MS)) {
      last_print_time = current_time;
      tracking_status_print(&inputs_state, control.filtered_error,
                            &targets);
    }
#endif
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(TRACKING_CONTROL_PERIOD_MS));
  }
}
