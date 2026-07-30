/* SPDX-License-Identifier: MIT */
/**
 * @file    app_emm_v5.c
 * @brief   EMM V5闭环步进电机恒速运行任务。
 * @author  weixin
 */
#include "app_emm_v5.h"

#include "emm_v5/board_emm_v5.h"
#include "motor.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 恒速测试目标，单位为RPM；保持低速以降低首次联调风险。 */
#define EMM_V5_TEST_SPEED_RPM INT32_C(60)
/* 恒速命令发送完成后的任务存活周期，单位为ms。 */
#define EMM_V5_KEEPALIVE_MS UINT32_C(1000)
/* 等待EMM V5完成上电自检和闭环初始化的时间，单位为ms。 */
#define EMM_V5_POWER_ON_DELAY_MS UINT32_C(2000)
/* 使能命令发送成功后，驱动器进入可调速状态所需的等待时间，单位为ms。 */
#define EMM_V5_ENABLE_SETTLE_DELAY_MS UINT32_C(10)

/**
 * @brief  使能EMM V5步进电机并发送一次低速恒速命令。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None，任务启动后持续运行，发生错误时主动删除自身。
 * @note   本任务独占板级EMM V5电机对象及其USART6通信资源。
 */
void emm_v5_task(void *argument)
{
  struct motor *motor;
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

  /* 给驱动器预留处理使能命令的时间，避免紧随其后的速度命令被忽略。 */
  vTaskDelay(pdMS_TO_TICKS(EMM_V5_ENABLE_SETTLE_DELAY_MS));
  result = motor_set_speed(motor, EMM_V5_TEST_SPEED_RPM);
  if (result != PLATFORM_OK) {
    printf("[emm_v5] speed failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }
  printf("[emm_v5] constant speed=%ld rpm\r\n",
         (long)EMM_V5_TEST_SPEED_RPM);

  /* 命令只需发送一次；驱动器会持续保持目标速度。 */
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(EMM_V5_KEEPALIVE_MS));
  }
}
