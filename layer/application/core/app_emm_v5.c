/* SPDX-License-Identifier: MIT */
/**
 * @file    app_emm_v5.c
 * @brief   EMM V5闭环步进电机使能和位置监测任务。
 * @author  weixin
 */
#include "app_emm_v5.h"
#include "app_serial_monitor.h"

#include "emm_v5/board_emm_v5.h"
#include "motor.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 编码器位置安全检查周期，单位为ms。 */
#define EMM_V5_CONTROL_PERIOD_MS UINT32_C(20)
/* 1表示周期打印position_raw，0表示关闭周期日志但继续执行限位检测。 */
#define EMM_V5_POSITION_RAW_LOG_ENABLED 0
#if EMM_V5_POSITION_RAW_LOG_ENABLED
/* 编码器位置日志周期，单位为ms。 */
#define EMM_V5_POSITION_LOG_PERIOD_MS UINT32_C(100)
#endif
/* 等待EMM V5完成上电自检和闭环初始化的时间，单位为ms。 */
#define EMM_V5_POWER_ON_DELAY_MS UINT32_C(2000)
/* 1表示执行WEIXIN的emm.position_deg目标，0表示保留参数但不控制EMM。 */
#define EMM_V5_WEIXIN_POSITION_CONTROL_ENABLED 0
/* 1表示执行球控任务发布的绝对位置目标。 */
#define EMM_V5_BALL_POSITION_CONTROL_ENABLED 1

#if (EMM_V5_WEIXIN_POSITION_CONTROL_ENABLED && \
     EMM_V5_BALL_POSITION_CONTROL_ENABLED)
#error "Only one EMM V5 absolute-position command source may be enabled"
#endif

/* 球控任务写入、EMM任务读取的绝对位置邮箱，单位为度。 */
static float s_target_position_deg;

/**
 * @brief  发布EMM绝对位置目标，由EMM任务通过USART6串行下发。
 * @param  position_deg 相对启动零点的绝对角度，单位为度。
 * @retval PLATFORM_OK表示目标已写入邮箱，其他值表示参数非法。
 * @note   临界区只保护32位目标值，不执行串口或电机操作。
 */
int emm_v5_target_position_deg_set(float position_deg)
{
  if ((position_deg != position_deg) ||
      (position_deg < BOARD_EMM_V5_POSITION_DEG_MIN) ||
      (position_deg > BOARD_EMM_V5_POSITION_DEG_MAX)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  taskENTER_CRITICAL();
  s_target_position_deg = position_deg;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

/**
 * @brief  获取球控任务最近发布的EMM绝对位置目标。
 * @param  position_deg 用于接收目标角度的变量，单位为度。
 * @retval None。
 */
static void emm_v5_target_position_deg_get(float *position_deg)
{
  taskENTER_CRITICAL();
  *position_deg = s_target_position_deg;
  taskEXIT_CRITICAL();
}

/**
 * @brief  在目标变化时发送经过软件限位检查的EMM绝对位置命令。
 * @param  position_raw 最近一次编码器原始位置，用于拦截日志。
 * @param  applied_position_deg 保存最近处理过的目标角度。
 * @param  position_blocked 保存上一条命令是否被软件限位拦截。
 * @retval PLATFORM_OK表示目标已处理，其他值表示电机通信或状态错误。
 */
static int emm_v5_ball_position_target_apply(
    int32_t position_raw,
    float *applied_position_deg,
    bool *position_blocked)
{
  float requested_position_deg;
  int result;

  emm_v5_target_position_deg_get(&requested_position_deg);
  if (requested_position_deg == *applied_position_deg) {
    return PLATFORM_OK;
  }
  result = board_emm_v5_position_deg_set(requested_position_deg);
  if (result == PLATFORM_ERR_BUSY) {
    if (!*position_blocked) {
      printf("[emm_v5] position blocked by limit: raw=%ld target=%.3f deg\r\n",
             (long)position_raw, (double)requested_position_deg);
    }
    *position_blocked = true;
    *applied_position_deg = requested_position_deg;
    return PLATFORM_OK;
  }
  if (result != PLATFORM_OK) {
    return result;
  }
  if (*position_blocked) {
    printf("[emm_v5] position command returned toward safe range\r\n");
  }
  *position_blocked = false;
  *applied_position_deg = requested_position_deg;
  return PLATFORM_OK;
}

/**
 * @brief  记录EMM运行期错误，尝试停机并删除当前任务。
 * @param  motor 已经启动的EMM电机对象。
 * @param  operation 失败操作的日志名称。
 * @param  result 失败操作返回的错误码。
 * @retval None。
 */
static void emm_v5_runtime_fail(struct motor *motor,
                                const char *operation,
                                int result)
{
  int stop_result;

  printf("[emm_v5] %s failed: %d\r\n", operation, result);
  stop_result = motor_stop(motor);
  if (stop_result != PLATFORM_OK) {
    printf("[emm_v5] emergency stop failed: %d\r\n", stop_result);
  }
  vTaskDelete(NULL);
}

/**
 * @brief  使能EMM V5步进电机并周期读取实时位置。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None，任务启动后持续运行，发生错误时主动删除自身。
 * @note   本任务独占板级EMM V5电机对象及其USART6通信资源。
 */
void emm_v5_task(void *argument)
{
  struct motor *motor;
  TickType_t last_wake_time;
#if EMM_V5_POSITION_RAW_LOG_ENABLED
  TickType_t last_log_tick;
  TickType_t current_tick;
#endif
  int32_t position_raw;
  bool limit_active;
  bool previous_limit_active;
#if EMM_V5_BALL_POSITION_CONTROL_ENABLED
  float applied_ball_position_deg = 1.0F;
  bool ball_position_blocked = false;
#endif
#if EMM_V5_WEIXIN_POSITION_CONTROL_ENABLED
  float requested_position_deg;
  float applied_position_deg = 0.0F;
#endif
  int result;

  (void)argument;

  result = board_emm_v5_init();
  if (result != PLATFORM_OK) {
    printf("[emm_v5] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }
  motor = board_emm_v5_get();
  if (motor == NULL) {
    printf("[emm_v5] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  /* 驱动器与MCU同时上电时，过早发送的串口命令会被启动流程直接丢弃。 */
  vTaskDelay(pdMS_TO_TICKS(EMM_V5_POWER_ON_DELAY_MS));
  result = motor_start(motor);
  if (result != PLATFORM_OK) {
    printf("[emm_v5] start failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  printf("[emm_v5] enabled\r\n");

  result = board_emm_v5_position_read(&position_raw);
  if (result != PLATFORM_OK) {
    emm_v5_runtime_fail(motor, "startup position read", result);
    return;
  }
  limit_active = board_emm_v5_position_is_limited();
  previous_limit_active = limit_active;

  if (!limit_active) {
    result = board_emm_v5_position_clear();
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "position clear", result);
      return;
    }
    position_raw = 0;
    printf("[emm_v5] startup position set to 0 deg\r\n");
  } else {
    printf("[emm_v5] startup outside limit; position not cleared\r\n");
  }

  last_wake_time = xTaskGetTickCount();
#if EMM_V5_POSITION_RAW_LOG_ENABLED
  last_log_tick = last_wake_time;
#endif
  for (;;) {
    result = board_emm_v5_position_read(&position_raw);
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "position read", result);
      return;
    }
    limit_active = board_emm_v5_position_is_limited();
    if (limit_active != previous_limit_active) {
      if (limit_active) {
        printf("[emm_v5] position limit reached: raw=%ld limit=%ld\r\n",
               (long)position_raw,
               (long)BOARD_EMM_V5_POSITION_RAW_ABS_LIMIT);
      } else {
        printf("[emm_v5] position returned inside limit: raw=%ld\r\n",
               (long)position_raw);
      }
      previous_limit_active = limit_active;
    }

#if EMM_V5_BALL_POSITION_CONTROL_ENABLED
    result = emm_v5_ball_position_target_apply(
        position_raw, &applied_ball_position_deg,
        &ball_position_blocked);
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "ball position set", result);
      return;
    }
#endif

#if EMM_V5_WEIXIN_POSITION_CONTROL_ENABLED
    result = serial_monitor_emm_position_deg_get(&requested_position_deg);
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "target read", result);
      return;
    }
    if (requested_position_deg != applied_position_deg) {
      result = board_emm_v5_position_deg_set(requested_position_deg);
      if (result == PLATFORM_OK) {
        printf("[emm_v5] target_deg=%.3f\r\n",
               (double)requested_position_deg);
      } else if (result == PLATFORM_ERR_BUSY) {
        printf("[emm_v5] target blocked by limit: raw=%ld target_deg=%.3f\r\n",
               (long)position_raw,
               (double)requested_position_deg);
      } else {
        emm_v5_runtime_fail(motor, "position set", result);
        return;
      }
      applied_position_deg = requested_position_deg;
    }
#endif
#if EMM_V5_POSITION_RAW_LOG_ENABLED
    current_tick = xTaskGetTickCount();
    if ((current_tick - last_log_tick) >=
        pdMS_TO_TICKS(EMM_V5_POSITION_LOG_PERIOD_MS)) {
      printf("[emm_v5] position_raw=%ld\r\n",
             (long)position_raw);
      last_log_tick = current_tick;
    }
#endif
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(EMM_V5_CONTROL_PERIOD_MS));
  }
}
