/* SPDX-License-Identifier: MIT */
#include "app_mission.h"

#include "app_ball_control.h"
#include "app_serial_monitor.h"
#include "app_tracking.h"
#include "result.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 竞赛任务总数，OLED选择编号范围为1至6。 */
#define MISSION_COUNT UINT8_C(6)
/* 按键事件队列最多缓存的事件数量。 */
#define MISSION_EVENT_QUEUE_LENGTH 8U
/* 任务状态机检查传感器和阶段切换的周期，单位为毫秒。 */
#define MISSION_MONITOR_PERIOD_MS UINT32_C(10)

/* 任务2子阶段：等待车辆完全离开起点的宽黑线。 */
#define MISSION_TASK2_WAIT_START_CLEAR UINT8_C(0)
/* 任务2子阶段：循迹并等待再次检测到终点宽黑线。 */
#define MISSION_TASK2_WAIT_FINISH_MARKER UINT8_C(1)
/* 任务2子阶段：执行第一段固定方向修正。 */
#define MISSION_TASK2_TURN_RIGHT UINT8_C(2)
/* 任务2子阶段：执行第二段倒退向右修正。 */
#define MISSION_TASK2_REVERSE_RIGHT UINT8_C(3)

/* 任务2固定修正总开关：1开启两段配置，0检测终点后直接停车。 */
#define MISSION_TASK2_FIXED_CORRECTION_ENABLE 1
/* 任务2第一段修正开关：1执行，0跳过。 */
#define MISSION_TASK2_RIGHT_TURN_ENABLE 1
/* 任务2第二段倒退向右修正开关：1执行，0跳过。 */
#define MISSION_TASK2_REVERSE_RIGHT_ENABLE 0
#if ((MISSION_TASK2_FIXED_CORRECTION_ENABLE != 0) && \
     (MISSION_TASK2_FIXED_CORRECTION_ENABLE != 1))
#error "MISSION_TASK2_FIXED_CORRECTION_ENABLE must be 0 or 1"
#endif
#if ((MISSION_TASK2_RIGHT_TURN_ENABLE != 0) && \
     (MISSION_TASK2_RIGHT_TURN_ENABLE != 1))
#error "MISSION_TASK2_RIGHT_TURN_ENABLE must be 0 or 1"
#endif
#if ((MISSION_TASK2_REVERSE_RIGHT_ENABLE != 0) && \
     (MISSION_TASK2_REVERSE_RIGHT_ENABLE != 1))
#error "MISSION_TASK2_REVERSE_RIGHT_ENABLE must be 0 or 1"
#endif
/* 任务2启动后允许判断终点停车的最短时间，单位为毫秒；0表示立即允许。 */
#define MISSION_TASK2_STOP_ENABLE_DELAY_MS UINT32_C(0)
/* 任务2第一段修正持续时间，单位为毫秒。 */
#define MISSION_TASK2_RIGHT_TURN_DURATION_MS UINT32_C(600)
/* 任务2第一段修正的左轮目标速度，单位为编码器计数每秒。 */
#define MISSION_TASK2_RIGHT_TURN_LEFT_SPEED INT32_C(-10000)
/* 任务2第一段修正的右轮目标速度，单位为编码器计数每秒。 */
#define MISSION_TASK2_RIGHT_TURN_RIGHT_SPEED INT32_C(-60000)
/* 任务2第二段倒退向右修正持续时间，单位为毫秒。 */
#define MISSION_TASK2_REVERSE_RIGHT_DURATION_MS UINT32_C(0000)
/* 任务2第二段修正的左轮目标速度，单位为编码器计数每秒。 */
#define MISSION_TASK2_REVERSE_RIGHT_LEFT_SPEED INT32_C(-10000)
/* 任务2第二段修正的右轮目标速度，单位为编码器计数每秒。 */
#define MISSION_TASK2_REVERSE_RIGHT_RIGHT_SPEED INT32_C(-80000)

/* 任务2循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK2_TRACKING_SPEED INT32_C(95000)
/* 任务4循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK4_TRACKING_SPEED INT32_C(60000)
/* 任务4启动后自动停止小车的时间，单位为毫秒。 */
#define MISSION_TASK4_AUTO_STOP_MS UINT32_C(8000)
/* 任务4的小球视觉X目标，2表示当前标定的摆杆中心点。 */
#define MISSION_TASK4_BALL_TARGET_X INT16_C(2)
/* 任务5循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK5_TRACKING_SPEED INT32_C(60000)
/* 任务5启动时从静止线性加速到目标速度的时间，单位为毫秒。 */
#define MISSION_TASK5_STARTUP_RAMP_MS UINT32_C(3000)
/* 任务5的小球视觉X目标，2表示当前标定的摆杆中心点。 */
#define MISSION_TASK5_BALL_TARGET_X INT16_C(2)
/* 任务6循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK6_TRACKING_SPEED INT32_C(60000)
/* 任务6启动时从静止线性加速到目标速度的时间，单位为毫秒。 */
#define MISSION_TASK6_STARTUP_RAMP_MS UINT32_C(3000)

/* 任务3前半段纯定时开环，后半段切换视觉位置闭环。 */
#define MISSION_TASK3_NEGATIVE_5CM_TARGET_X INT16_C(-125)
#define MISSION_TASK3_DRIVE_POSITIVE UINT8_C(0)
#define MISSION_TASK3_BRAKE_POSITIVE UINT8_C(1)
#define MISSION_TASK3_HOLD_POSITIVE UINT8_C(2)
#define MISSION_TASK3_DRIVE_NEGATIVE UINT8_C(3)
#define MISSION_TASK3_BRAKE_NEGATIVE UINT8_C(4)
#define MISSION_TASK3_HOLD_NEGATIVE UINT8_C(5)
#define MISSION_TASK3_CLOSED_LOOP_NEGATIVE UINT8_C(6)

/* 任务6子阶段：车辆静止并采集评委指定的小球位置。 */
#define MISSION_TASK6_SAMPLE_TARGET UINT8_C(0)
/* 任务6子阶段：保持采样目标并缓慢启动循迹。 */
#define MISSION_TASK6_TRACK_AND_HOLD UINT8_C(1)
/* 任务6用于计算目标位置平均值的有效视觉新帧数量。 */
#define MISSION_TASK6_TARGET_SAMPLE_FRAMES UINT8_C(30)

static const int8_t s_task2_tracking_weights[APP_TRACKING_WEIGHT_COUNT] = {
  -15, -10, -6, -6, 6, 6, 10, 15
};
static const int8_t s_task4_tracking_weights[APP_TRACKING_WEIGHT_COUNT] = {
  -8, -6, -4, -3, 3, 4, 6, 8
};
static const int8_t s_task5_tracking_weights[APP_TRACKING_WEIGHT_COUNT] = {
  -8, -6, -4, -3, 3, 4, 6, 8
};
static const int8_t s_task6_tracking_weights[APP_TRACKING_WEIGHT_COUNT] = {
  -8, -6, -4, -3, 3, 4, 6, 8
};

static QueueHandle_t s_event_queue;
static struct mission_snapshot s_snapshot = {
  .selected_task = UINT8_C(1),
  .state = MISSION_STATE_SELECTING,
  .substep = UINT8_C(0),
  .elapsed_ms = UINT32_C(0),
  .error_code = PLATFORM_OK,
  .revision = UINT32_C(1)
};
static TickType_t s_started_tick;
static TickType_t s_task2_phase_started_tick;
static TickType_t s_task3_phase_started_tick;
static uint32_t s_task3_closed_loop_start_sequence;
static struct serial_monitor_task3_open_loop_config s_task3_config;
static bool s_tracking_startup_ramp_complete = true;
static TickType_t s_tracking_startup_ramp_started_tick;
static int32_t s_task6_target_position_sum;
static uint32_t s_task6_last_vision_sequence;
static uint8_t s_task6_target_sample_count;

static const char *const s_task_names[MISSION_COUNT] = {
  "VISION RECORD",
  "TRACK LAP",
  "BALL +/-5CM",
  "TRACK TO B",
  "LAP + BALL",
  "LAP BALL POS"
};

int mission_init(void)
{
  if (s_event_queue != NULL) {
    return PLATFORM_OK;
  }
  s_event_queue = xQueueCreate(MISSION_EVENT_QUEUE_LENGTH,
                               sizeof(enum mission_key_event));
  return (s_event_queue != NULL) ? PLATFORM_OK : PLATFORM_ERR_IO;
}

int mission_key_event_post(enum mission_key_event event)
{
  if (s_event_queue == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((event != MISSION_KEY_SHORT_RELEASE) &&
      (event != MISSION_KEY_LONG_RELEASE)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  return (xQueueSend(s_event_queue, &event, 0U) == pdPASS) ?
         PLATFORM_OK : PLATFORM_ERR_BUSY;
}

int mission_snapshot_get(struct mission_snapshot *snapshot)
{
  if (snapshot == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  taskENTER_CRITICAL();
  *snapshot = s_snapshot;
  if (snapshot->state == MISSION_STATE_RUNNING) {
    snapshot->elapsed_ms = (uint32_t)
        ((xTaskGetTickCount() - s_started_tick) * portTICK_PERIOD_MS);
  }
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

const char *mission_task_name_get(uint8_t task_number)
{
  if ((task_number < UINT8_C(1)) ||
      (task_number > MISSION_COUNT)) {
    return "UNKNOWN";
  }
  return s_task_names[task_number - UINT8_C(1)];
}

const char *mission_state_name_get(enum mission_state state)
{
  switch (state) {
  case MISSION_STATE_SELECTING:
    return "SELECTING";
  case MISSION_STATE_RUNNING:
    return "RUNNING";
  case MISSION_STATE_COMPLETE:
    return "COMPLETE";
  case MISSION_STATE_TIMEOUT:
    return "TIMEOUT";
  case MISSION_STATE_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

static int mission_outputs_stop(void)
{
  int tracking_result;
  int ball_result;

  tracking_result = tracking_enabled_set(false);
  ball_result = ball_control_enabled_set(false);
  return (tracking_result != PLATFORM_OK) ? tracking_result : ball_result;
}

static int mission_tracking_start(
    int32_t speed,
    const int8_t weights[APP_TRACKING_WEIGHT_COUNT],
    uint16_t initial_scale_per_mille)
{
  int result = tracking_weights_set(weights);

  if (result == PLATFORM_OK) {
    result = tracking_base_speeds_set(speed, speed);
  }
  if (result == PLATFORM_OK) {
    result = tracking_output_scale_set(initial_scale_per_mille);
  }
  if (result == PLATFORM_OK) {
    result = tracking_enabled_set(true);
  }
  return result;
}

static int mission_task6_start(void)
{
  struct ball_control_status ball_status;
  int result;

  s_task6_target_position_sum = INT32_C(0);
  s_task6_target_sample_count = UINT8_C(0);
  result = ball_control_status_get(&ball_status);
  if (result == PLATFORM_OK) {
    s_task6_last_vision_sequence = ball_status.vision_sequence;
    result = mission_tracking_start(MISSION_TASK6_TRACKING_SPEED,
                                    s_task6_tracking_weights,
                                    UINT16_C(0));
  }
  return result;
}

static int mission_selected_start(uint8_t selected_task)
{
  int result;

  result = mission_outputs_stop();
  if (result != PLATFORM_OK) {
    return result;
  }

  switch (selected_task) {
  case UINT8_C(1):
    return PLATFORM_OK;
  case UINT8_C(2):
    return mission_tracking_start(MISSION_TASK2_TRACKING_SPEED,
                                  s_task2_tracking_weights,
                                  APP_TRACKING_OUTPUT_SCALE_MAX);
  case UINT8_C(3):
    result = serial_monitor_task3_open_loop_config_get(
        &s_task3_config);
    if (result == PLATFORM_OK) {
      s_task3_phase_started_tick = xTaskGetTickCount();
      result = ball_control_open_loop_position_deg_set(
          s_task3_config.positive_drive_angle_deg);
    }
    return result;
  case UINT8_C(4):
  case UINT8_C(5):
    result = ball_control_target_x_set(
        (selected_task == UINT8_C(4)) ?
        MISSION_TASK4_BALL_TARGET_X :
        MISSION_TASK5_BALL_TARGET_X);
    if (result == PLATFORM_OK) {
      result = ball_control_enabled_set(true);
    }
    if (result == PLATFORM_OK) {
      result = mission_tracking_start(
          (selected_task == UINT8_C(4)) ?
          MISSION_TASK4_TRACKING_SPEED :
          MISSION_TASK5_TRACKING_SPEED,
          (selected_task == UINT8_C(4)) ?
          s_task4_tracking_weights :
          s_task5_tracking_weights,
          (selected_task == UINT8_C(4)) ?
          APP_TRACKING_OUTPUT_SCALE_MAX : UINT16_C(0));
    }
    return result;
  case UINT8_C(6):
    return mission_task6_start();
  default:
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
}

static void mission_select_next(void)
{
  taskENTER_CRITICAL();
  s_snapshot.selected_task =
      (s_snapshot.selected_task >= MISSION_COUNT) ?
      UINT8_C(1) : (uint8_t)(s_snapshot.selected_task + UINT8_C(1));
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
}

static void mission_start(void)
{
  uint8_t selected_task;
  int result;

  taskENTER_CRITICAL();
  selected_task = s_snapshot.selected_task;
  taskEXIT_CRITICAL();

  result = mission_selected_start(selected_task);
  taskENTER_CRITICAL();
  s_snapshot.substep = UINT8_C(0);
  s_snapshot.elapsed_ms = UINT32_C(0);
  s_snapshot.error_code = result;
  s_snapshot.state = (result == PLATFORM_OK) ?
                     MISSION_STATE_RUNNING : MISSION_STATE_ERROR;
  s_started_tick = xTaskGetTickCount();
  s_tracking_startup_ramp_started_tick = s_started_tick;
  s_tracking_startup_ramp_complete =
      (selected_task != UINT8_C(5)) &&
      (selected_task != UINT8_C(6));
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  printf("[mission] task %u %s\r\n", (unsigned int)selected_task,
         (result == PLATFORM_OK) ? "started" : "start failed");
}

static void mission_abort(void)
{
  int result = mission_outputs_stop();

  taskENTER_CRITICAL();
  s_snapshot.state = (result == PLATFORM_OK) ?
                     MISSION_STATE_SELECTING : MISSION_STATE_ERROR;
  s_snapshot.substep = UINT8_C(0);
  s_snapshot.elapsed_ms = UINT32_C(0);
  s_snapshot.error_code = result;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  printf("[mission] stopped\r\n");
}

static void mission_complete(bool keep_ball_control)
{
  uint32_t elapsed_ms = (uint32_t)
      ((xTaskGetTickCount() - s_started_tick) * portTICK_PERIOD_MS);
  int result;

  if (keep_ball_control) {
    result = tracking_enabled_set(false);
  } else {
    result = mission_outputs_stop();
  }

  taskENTER_CRITICAL();
  s_snapshot.elapsed_ms = elapsed_ms;
  s_snapshot.error_code = result;
  s_snapshot.state = (result == PLATFORM_OK) ?
                     MISSION_STATE_COMPLETE : MISSION_STATE_ERROR;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  printf("[mission] task complete, time=%lu ms\r\n",
         (unsigned long)elapsed_ms);
}

static void mission_error_set(int error_code)
{
  (void)mission_outputs_stop();
  taskENTER_CRITICAL();
  s_snapshot.error_code = error_code;
  s_snapshot.state = MISSION_STATE_ERROR;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
}

static bool mission_task2_stop_window_open(TickType_t elapsed_ticks)
{
#if (MISSION_TASK2_STOP_ENABLE_DELAY_MS == 0U)
  (void)elapsed_ticks;
  return true;
#else
  return elapsed_ticks >=
         pdMS_TO_TICKS(MISSION_TASK2_STOP_ENABLE_DELAY_MS);
#endif
}

static int mission_task2_manual_phase_start(
    int32_t left_speed,
    int32_t right_speed,
    uint8_t substep)
{
  int result = tracking_manual_speeds_set(left_speed, right_speed);

  if (result != PLATFORM_OK) {
    return result;
  }
  s_task2_phase_started_tick = xTaskGetTickCount();
  taskENTER_CRITICAL();
  s_snapshot.substep = substep;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

static void mission_task2_update(uint8_t substep)
{
  struct tracking_status tracking_status;
  TickType_t elapsed_ticks;
  int result;

  if ((substep == MISSION_TASK2_WAIT_START_CLEAR) ||
      (substep == MISSION_TASK2_WAIT_FINISH_MARKER)) {
    if ((tracking_status_get(&tracking_status) != PLATFORM_OK) ||
        !tracking_status.sample_valid) {
      return;
    }
  }

  if (substep == MISSION_TASK2_WAIT_START_CLEAR) {
    if (tracking_status.stop_marker_clear_confirmed) {
      taskENTER_CRITICAL();
      s_snapshot.substep = MISSION_TASK2_WAIT_FINISH_MARKER;
      s_snapshot.revision++;
      taskEXIT_CRITICAL();
      printf("[mission] task 2 start marker cleared\r\n");
    }
  } else if (substep == MISSION_TASK2_WAIT_FINISH_MARKER) {
    elapsed_ticks = xTaskGetTickCount() - s_started_tick;
    if (mission_task2_stop_window_open(elapsed_ticks) &&
        tracking_status.stop_marker_detected) {
#if (MISSION_TASK2_FIXED_CORRECTION_ENABLE == 1)
#if (MISSION_TASK2_RIGHT_TURN_ENABLE == 1)
      result = mission_task2_manual_phase_start(
          MISSION_TASK2_RIGHT_TURN_LEFT_SPEED,
          MISSION_TASK2_RIGHT_TURN_RIGHT_SPEED,
          MISSION_TASK2_TURN_RIGHT);
#elif (MISSION_TASK2_REVERSE_RIGHT_ENABLE == 1)
      result = mission_task2_manual_phase_start(
          MISSION_TASK2_REVERSE_RIGHT_LEFT_SPEED,
          MISSION_TASK2_REVERSE_RIGHT_RIGHT_SPEED,
          MISSION_TASK2_REVERSE_RIGHT);
#else
      result = PLATFORM_OK;
      mission_complete(false);
#endif
      if (result != PLATFORM_OK) {
        mission_error_set(result);
        return;
      }
      printf("[mission] task 2 finish marker, correcting heading\r\n");
#else
      mission_complete(false);
#endif
    }
  } else if (substep == MISSION_TASK2_TURN_RIGHT) {
    elapsed_ticks = xTaskGetTickCount() - s_task2_phase_started_tick;
    if (elapsed_ticks >=
        pdMS_TO_TICKS(MISSION_TASK2_RIGHT_TURN_DURATION_MS)) {
#if (MISSION_TASK2_REVERSE_RIGHT_ENABLE == 1)
      result = mission_task2_manual_phase_start(
          MISSION_TASK2_REVERSE_RIGHT_LEFT_SPEED,
          MISSION_TASK2_REVERSE_RIGHT_RIGHT_SPEED,
          MISSION_TASK2_REVERSE_RIGHT);
      if (result != PLATFORM_OK) {
        mission_error_set(result);
        return;
      }
#else
      mission_complete(false);
#endif
    }
#if (MISSION_TASK2_REVERSE_RIGHT_ENABLE == 1)
  } else if (substep == MISSION_TASK2_REVERSE_RIGHT) {
    elapsed_ticks = xTaskGetTickCount() - s_task2_phase_started_tick;
    if (elapsed_ticks >=
        pdMS_TO_TICKS(MISSION_TASK2_REVERSE_RIGHT_DURATION_MS)) {
      result = tracking_enabled_set(false);
      if (result != PLATFORM_OK) {
        mission_error_set(result);
        return;
      }
      mission_complete(false);
    }
#endif
  }
}

static int mission_task3_phase_start(uint8_t substep,
                                     float position_deg)
{
  int result = ball_control_open_loop_position_deg_set(position_deg);

  if (result != PLATFORM_OK) {
    return result;
  }
  s_task3_phase_started_tick = xTaskGetTickCount();
  taskENTER_CRITICAL();
  s_snapshot.substep = substep;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

static int mission_task3_closed_loop_start(void)
{
  struct ball_control_status ball_status;
  int result = ball_control_status_get(&ball_status);

  if (result == PLATFORM_OK) {
    s_task3_closed_loop_start_sequence = ball_status.vision_sequence;
    result = ball_control_target_x_set(
        MISSION_TASK3_NEGATIVE_5CM_TARGET_X);
  }
  if (result == PLATFORM_OK) {
    result = ball_control_enabled_set(true);
  }
  if (result == PLATFORM_OK) {
    taskENTER_CRITICAL();
    s_snapshot.substep = MISSION_TASK3_CLOSED_LOOP_NEGATIVE;
    s_snapshot.revision++;
    taskEXIT_CRITICAL();
  }
  return result;
}

static void mission_task3_update(uint8_t substep)
{
  struct ball_control_status ball_status;
  TickType_t current_tick = xTaskGetTickCount();
  uint32_t phase_duration_ms;
  uint8_t next_substep;
  float next_position_deg;
  int result;

  if (substep == MISSION_TASK3_DRIVE_POSITIVE) {
    phase_duration_ms = s_task3_config.positive_drive_time_ms;
    next_substep = MISSION_TASK3_BRAKE_POSITIVE;
    next_position_deg = s_task3_config.positive_brake_angle_deg;
  } else if (substep == MISSION_TASK3_BRAKE_POSITIVE) {
    phase_duration_ms = s_task3_config.positive_brake_time_ms;
    next_substep = MISSION_TASK3_HOLD_POSITIVE;
    next_position_deg = s_task3_config.positive_hold_angle_deg;
  } else if (substep == MISSION_TASK3_HOLD_POSITIVE) {
    phase_duration_ms = s_task3_config.positive_hold_time_ms;
    next_substep = MISSION_TASK3_DRIVE_NEGATIVE;
    next_position_deg = s_task3_config.negative_drive_angle_deg;
  } else if (substep == MISSION_TASK3_DRIVE_NEGATIVE) {
    phase_duration_ms = s_task3_config.negative_drive_time_ms;
    next_substep = MISSION_TASK3_BRAKE_NEGATIVE;
    next_position_deg = s_task3_config.negative_brake_angle_deg;
  } else if (substep == MISSION_TASK3_BRAKE_NEGATIVE) {
    phase_duration_ms = s_task3_config.negative_brake_time_ms;
    next_substep = MISSION_TASK3_HOLD_NEGATIVE;
    next_position_deg = s_task3_config.hold_angle_deg;
  } else if (substep == MISSION_TASK3_HOLD_NEGATIVE) {
    phase_duration_ms = s_task3_config.hold_time_ms;
    next_substep = MISSION_TASK3_CLOSED_LOOP_NEGATIVE;
    next_position_deg = 0.0F;
  } else if (substep == MISSION_TASK3_CLOSED_LOOP_NEGATIVE) {
    result = ball_control_status_get(&ball_status);
    if ((result == PLATFORM_OK) && ball_status.enabled &&
        ball_status.vision_valid && ball_status.arrived &&
        (ball_status.target_x ==
         MISSION_TASK3_NEGATIVE_5CM_TARGET_X) &&
        (ball_status.vision_sequence !=
         s_task3_closed_loop_start_sequence)) {
      mission_complete(true);
    } else if (result != PLATFORM_OK) {
      mission_error_set(result);
    }
    return;
  } else {
    mission_error_set(PLATFORM_ERR_INVALID_STATE);
    return;
  }

  if ((current_tick - s_task3_phase_started_tick) <
      pdMS_TO_TICKS(phase_duration_ms)) {
    return;
  }
  if (substep == MISSION_TASK3_HOLD_NEGATIVE) {
    result = mission_task3_closed_loop_start();
  } else {
    result = mission_task3_phase_start(next_substep,
                                       next_position_deg);
  }
  if (result != PLATFORM_OK) {
    mission_error_set(result);
  }
}

static void mission_task4_update(void)
{
  TickType_t elapsed_ticks = xTaskGetTickCount() - s_started_tick;

  if (elapsed_ticks >= pdMS_TO_TICKS(MISSION_TASK4_AUTO_STOP_MS)) {
    /* Keep ball control enabled at the centre after the car stops. */
    mission_complete(true);
  }
}

static void mission_tracking_startup_update(uint32_t ramp_duration_ms)
{
  TickType_t elapsed_ticks;
  TickType_t ramp_ticks;
  uint16_t scale_per_mille;
  int result;

  if (s_tracking_startup_ramp_complete) {
    return;
  }
  elapsed_ticks =
      xTaskGetTickCount() - s_tracking_startup_ramp_started_tick;
  ramp_ticks = pdMS_TO_TICKS(ramp_duration_ms);
  if ((ramp_ticks == 0U) || (elapsed_ticks >= ramp_ticks)) {
    scale_per_mille = APP_TRACKING_OUTPUT_SCALE_MAX;
  } else {
    scale_per_mille = (uint16_t)(
        ((uint32_t)elapsed_ticks *
         (uint32_t)APP_TRACKING_OUTPUT_SCALE_MAX) /
        (uint32_t)ramp_ticks);
  }
  result = tracking_output_scale_set(scale_per_mille);
  if (result != PLATFORM_OK) {
    mission_error_set(result);
    return;
  }
  if (scale_per_mille >= APP_TRACKING_OUTPUT_SCALE_MAX) {
    s_tracking_startup_ramp_complete = true;
  }
}

static int16_t mission_task6_target_average_get(void)
{
  int32_t rounded_sum = s_task6_target_position_sum;
  int32_t half_sample_count =
      (int32_t)MISSION_TASK6_TARGET_SAMPLE_FRAMES / INT32_C(2);

  rounded_sum += (rounded_sum >= INT32_C(0)) ?
                 half_sample_count : -half_sample_count;
  return (int16_t)(rounded_sum /
                   (int32_t)MISSION_TASK6_TARGET_SAMPLE_FRAMES);
}

static void mission_task6_update(uint8_t substep)
{
  struct ball_control_status ball_status;
  int16_t target_x;
  int result;

  if (substep == MISSION_TASK6_TRACK_AND_HOLD) {
    mission_tracking_startup_update(MISSION_TASK6_STARTUP_RAMP_MS);
    return;
  }
  if (substep != MISSION_TASK6_SAMPLE_TARGET) {
    mission_error_set(PLATFORM_ERR_INVALID_STATE);
    return;
  }
  result = ball_control_status_get(&ball_status);
  if ((result != PLATFORM_OK) || !ball_status.vision_valid ||
      (ball_status.vision_sequence == s_task6_last_vision_sequence)) {
    return;
  }
  s_task6_last_vision_sequence = ball_status.vision_sequence;
  s_task6_target_position_sum += (int32_t)ball_status.measured_x;
  s_task6_target_sample_count++;
  if (s_task6_target_sample_count <
      MISSION_TASK6_TARGET_SAMPLE_FRAMES) {
    return;
  }

  target_x = mission_task6_target_average_get();
  result = ball_control_target_x_set(target_x);
  if (result == PLATFORM_OK) {
    result = ball_control_enabled_set(true);
  }
  if (result != PLATFORM_OK) {
    mission_error_set(result);
    return;
  }
  s_tracking_startup_ramp_started_tick = xTaskGetTickCount();
  s_tracking_startup_ramp_complete = false;
  taskENTER_CRITICAL();
  s_snapshot.substep = MISSION_TASK6_TRACK_AND_HOLD;
  s_snapshot.revision++;
  taskEXIT_CRITICAL();
  printf("[mission] task 6 target_x=%d from %u frames\r\n",
         (int)target_x,
         (unsigned int)MISSION_TASK6_TARGET_SAMPLE_FRAMES);
}

static void mission_running_update(void)
{
  uint8_t selected_task;
  uint8_t substep;
  enum mission_state state;

  taskENTER_CRITICAL();
  state = s_snapshot.state;
  selected_task = s_snapshot.selected_task;
  substep = s_snapshot.substep;
  taskEXIT_CRITICAL();

  if (state != MISSION_STATE_RUNNING) {
    return;
  }
  if (selected_task == UINT8_C(2)) {
    mission_task2_update(substep);
  } else if (selected_task == UINT8_C(3)) {
    mission_task3_update(substep);
  } else if (selected_task == UINT8_C(4)) {
    mission_task4_update();
  } else if (selected_task == UINT8_C(5)) {
    mission_tracking_startup_update(MISSION_TASK5_STARTUP_RAMP_MS);
  } else if (selected_task == UINT8_C(6)) {
    mission_task6_update(substep);
  }
}

void mission_task(void *argument)
{
  enum mission_key_event event;
  enum mission_state state;

  (void)argument;
  if (s_event_queue == NULL) {
    printf("[mission] queue not initialized\r\n");
    vTaskDelete(NULL);
    return;
  }
  (void)mission_outputs_stop();

  for (;;) {
    if (xQueueReceive(s_event_queue, &event,
                      pdMS_TO_TICKS(MISSION_MONITOR_PERIOD_MS)) == pdPASS) {
      taskENTER_CRITICAL();
      state = s_snapshot.state;
      taskEXIT_CRITICAL();

      if ((event == MISSION_KEY_SHORT_RELEASE) &&
          (state == MISSION_STATE_SELECTING)) {
        mission_select_next();
      } else if (event == MISSION_KEY_LONG_RELEASE) {
        if (state == MISSION_STATE_SELECTING) {
          mission_start();
        } else {
          mission_abort();
        }
      }
    }
    mission_running_update();
  }
}
