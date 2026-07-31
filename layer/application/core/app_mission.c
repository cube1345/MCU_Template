/* SPDX-License-Identifier: MIT */
#include "app_mission.h"

#include "app_ball_control.h"
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
/* 任务4的小球视觉X目标，0表示摆杆中心点。 */
#define MISSION_TASK4_BALL_TARGET_X INT16_C(0)
/* 任务5循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK5_TRACKING_SPEED INT32_C(60000)
/* 任务5的小球视觉X目标，0表示摆杆中心点。 */
#define MISSION_TASK5_BALL_TARGET_X INT16_C(0)
/* 任务6循迹时左右轮的平均基础速度，单位为编码器计数每秒。 */
#define MISSION_TASK6_TRACKING_SPEED INT32_C(60000)

/* 任务3机械+5cm对应的视觉X目标；相机坐标方向与机械方向相反。 */
#define MISSION_TASK3_POSITIVE_5CM_TARGET_X INT16_C(-130)
/* 任务3机械-5cm对应的视觉X目标。 */
#define MISSION_TASK3_NEGATIVE_5CM_TARGET_X INT16_C(130)
/* 任务3子阶段：小球从中心前往机械+5cm位置。 */
#define MISSION_TASK3_GO_POSITIVE_5CM UINT8_C(0)
/* 任务3子阶段：小球折返并前往机械-5cm位置。 */
#define MISSION_TASK3_GO_NEGATIVE_5CM UINT8_C(1)

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
static uint32_t s_task3_target_start_sequence;

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
    const int8_t weights[APP_TRACKING_WEIGHT_COUNT])
{
  int result = tracking_weights_set(weights);

  if (result == PLATFORM_OK) {
    result = tracking_base_speeds_set(speed, speed);
  }
  if (result == PLATFORM_OK) {
    result = tracking_enabled_set(true);
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
                                  s_task2_tracking_weights);
  case UINT8_C(3):
    {
      struct ball_control_status ball_status;

      result = ball_control_status_get(&ball_status);
      if (result == PLATFORM_OK) {
        s_task3_target_start_sequence = ball_status.vision_sequence;
        result = ball_control_target_x_set(
            MISSION_TASK3_POSITIVE_5CM_TARGET_X);
      }
      if (result == PLATFORM_OK) {
        result = ball_control_enabled_set(true);
      }
      return result;
    }
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
          s_task5_tracking_weights);
    }
    return result;
  case UINT8_C(6):
    result = ball_control_enabled_set(true);
    if (result == PLATFORM_OK) {
      result = mission_tracking_start(MISSION_TASK6_TRACKING_SPEED,
                                      s_task6_tracking_weights);
    }
    return result;
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

static void mission_task3_update(uint8_t substep)
{
  struct ball_control_status ball_status;
  int result;

  result = ball_control_status_get(&ball_status);
  if ((result != PLATFORM_OK) || !ball_status.enabled ||
      !ball_status.vision_valid || !ball_status.arrived ||
      (ball_status.vision_sequence == s_task3_target_start_sequence)) {
    return;
  }

  if ((substep == MISSION_TASK3_GO_POSITIVE_5CM) &&
      (ball_status.target_x == MISSION_TASK3_POSITIVE_5CM_TARGET_X)) {
    s_task3_target_start_sequence = ball_status.vision_sequence;
    result = ball_control_target_x_set(
        MISSION_TASK3_NEGATIVE_5CM_TARGET_X);
    if (result != PLATFORM_OK) {
      taskENTER_CRITICAL();
      s_snapshot.error_code = result;
      s_snapshot.state = MISSION_STATE_ERROR;
      s_snapshot.revision++;
      taskEXIT_CRITICAL();
      (void)mission_outputs_stop();
      return;
    }
    taskENTER_CRITICAL();
    s_snapshot.substep = MISSION_TASK3_GO_NEGATIVE_5CM;
    s_snapshot.revision++;
    taskEXIT_CRITICAL();
    printf("[mission] task 3 reached +5 cm, returning\r\n");
  } else if ((substep == MISSION_TASK3_GO_NEGATIVE_5CM) &&
             (ball_status.target_x ==
              MISSION_TASK3_NEGATIVE_5CM_TARGET_X)) {
    mission_complete(true);
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
