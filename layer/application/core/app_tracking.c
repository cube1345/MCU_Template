/**
 * @file    app_tracking.c
 * @brief   六路循迹差速控制任务。
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
/* 循迹状态串口打印周期，单位为毫秒。 */
#define TRACKING_PRINT_PERIOD_MS UINT32_C(200)
/* 每个位置误差单位对应的轮速差，单位为编码器计数每秒。 */
#define TRACKING_DIFFERENTIAL_GAIN INT32_C(4000)
/* 循迹任务允许发布的单轮目标速度绝对值。 */
#define TRACKING_TARGET_SPEED_LIMIT INT32_C(200000)
/* 短时丢线保持上次方向的采样次数，对应 100 ms。 */
#define TRACKING_LOST_STOP_SAMPLES UINT32_C(50)
/* 参与控制的第一个通道，io0 和 io7 不参与循迹计算。 */
#define TRACKING_FIRST_CHANNEL UINT8_C(1)
/* 参与控制的中间循迹通道数量。 */
#define TRACKING_USED_CHANNELS UINT8_C(6)

/* io1 至 io6 从左到右的位置权重，负值表示线位于车体左侧。 */
static const int8_t s_tracking_weights[TRACKING_USED_CHANNELS] = {
  -9, -7, -1, 1, 7, 9
};

/** @brief 循迹控制器跨采样周期保留的丢线状态。 */
struct tracking_control_state {
  int32_t last_error;       /* 最近一次检测到黑线时的位置误差。 */
  uint32_t lost_samples;    /* 连续未检测到黑线的采样次数。 */
  bool line_seen;           /* 启动后是否至少检测到过一次黑线。 */
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
 * @brief  根据中间六路黑线输入计算加权位置误差。
 * @param  inputs 八路原始输入，低电平表示检测到黑线。
 * @param  error 用于接收位置误差，范围为 -5 至 5。
 * @return true 表示检测到黑线，false 表示中间六路均为白色。
 * @note   六路全黑按路口处理，误差设为零以保持直行。
 */
static bool tracking_error_calculate(
    const struct tracking_inputs_state *inputs,
    int32_t *error)
{
  int32_t weight_sum = 0;
  uint8_t black_count = UINT8_C(0);
  uint8_t index;

  for (index = UINT8_C(0);
       index < TRACKING_USED_CHANNELS; index++) {
    if (!inputs->high[index + TRACKING_FIRST_CHANNEL]) {
      weight_sum += s_tracking_weights[index];
      black_count++;
    }
  }
  if (black_count == UINT8_C(0)) {
    return false;
  }

  *error = (black_count == TRACKING_USED_CHANNELS) ?
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
  int32_t differential;

  if (line_found) {
    control->last_error = measured_error;
    control->lost_samples = UINT32_C(0);
    control->line_seen = true;
  } else if (control->lost_samples < UINT32_MAX) {
    control->lost_samples++;
  }

  if ((!control->line_seen) ||
      (control->lost_samples >= TRACKING_LOST_STOP_SAMPLES)) {
    targets->left_speed = INT32_C(0);
    targets->right_speed = INT32_C(0);
    return;
  }

  differential = control->last_error * TRACKING_DIFFERENTIAL_GAIN;
  targets->left_speed = tracking_speed_limit(
      base_speeds->left_speed + differential);
  targets->right_speed = tracking_speed_limit(
      base_speeds->right_speed - differential);
}

/**
 * @brief  低频打印八路输入以及当前循迹控制结果。
 * @param  inputs 八路循迹输入快照。
 * @param  error 当前采用的位置误差。
 * @param  targets 当前发布的左右轮目标速度。
 * @return 无返回值。
 */
static void tracking_status_print(
    const struct tracking_inputs_state *inputs,
    int32_t error,
    const struct tracking_wheel_targets *targets)
{
  printf("[tracking] %u%u%u%u%u%u%u%u mask=0x%02X "
         "error=%ld target=%ld,%ld\r\n",
         (unsigned int)inputs->high[0],
         (unsigned int)inputs->high[1],
         (unsigned int)inputs->high[2],
         (unsigned int)inputs->high[3],
         (unsigned int)inputs->high[4],
         (unsigned int)inputs->high[5],
         (unsigned int)inputs->high[6],
         (unsigned int)inputs->high[7],
         (unsigned int)inputs->high_mask,
         (long)error,
         (long)targets->left_speed,
         (long)targets->right_speed);
}

/**
 * @brief  周期采样中间六路循迹输入并发布左右轮目标速度。
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
  TickType_t last_print_time;
  TickType_t last_wake_time;
  TickType_t current_time;
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
  last_print_time = last_wake_time;
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

    current_time = xTaskGetTickCount();
    if ((TickType_t)(current_time - last_print_time) >=
        pdMS_TO_TICKS(TRACKING_PRINT_PERIOD_MS)) {
      last_print_time = current_time;
      tracking_status_print(&inputs_state, control.last_error,
                            &targets);
    }
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(TRACKING_CONTROL_PERIOD_MS));
  }
}
