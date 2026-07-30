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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 实时位置串口查询周期，单位为ms。 */
#define EMM_V5_POSITION_READ_PERIOD_MS UINT32_C(100)
/* 等待EMM V5完成上电自检和闭环初始化的时间，单位为ms。 */
#define EMM_V5_POWER_ON_DELAY_MS UINT32_C(2000)

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
  int32_t position_raw;
  float requested_position_deg;
  float applied_position_deg = 0.0F;
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

  result = board_emm_v5_position_clear();
  if (result != PLATFORM_OK) {
    emm_v5_runtime_fail(motor, "position clear", result);
    return;
  }
  printf("[emm_v5] startup position set to 0 deg\r\n");

  last_wake_time = xTaskGetTickCount();
  for (;;) {
    result = serial_monitor_emm_position_deg_get(
        &requested_position_deg);
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "target read", result);
      return;
    }
    if (requested_position_deg != applied_position_deg) {
      result = board_emm_v5_position_deg_set(requested_position_deg);
      if (result != PLATFORM_OK) {
        emm_v5_runtime_fail(motor, "position set", result);
        return;
      }
      applied_position_deg = requested_position_deg;
      printf("[emm_v5] target_deg=%.3f\r\n",
             (double)applied_position_deg);
    }
    result = board_emm_v5_position_read(&position_raw);
    if (result != PLATFORM_OK) {
      emm_v5_runtime_fail(motor, "position read", result);
      return;
    }
    printf("[emm_v5] position_raw=%ld\r\n",
           (long)position_raw);
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(EMM_V5_POSITION_READ_PERIOD_MS));
  }
}
