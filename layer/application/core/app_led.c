/**
 * @file    app_led.c
 * @brief   板载蓝色 LED 周期翻转应用。
 * @author  weixin
 */
#include "app_led.h"

#include "board_led.h"
#include "led.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 板载蓝色 LED 的翻转周期，单位为毫秒。 */
#define LED_PERIOD_MS UINT32_C(1000)

/**
 * @brief  运行板载蓝色 LED 周期翻转任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务负责初始化并独占板载蓝色 LED 对象。
 */
void led_task(void *argument)
{
  struct led *blue;
  TickType_t last_wake_time;
  int result;

  (void)argument;

  result = board_led_init();
  if (result != PLATFORM_OK) {
    printf("[led] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  blue = board_led_blue_get();
  if (blue == NULL) {
    printf("[led] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  for (;;) {
    /* 当前示例只演示周期翻转，不保存 LED 操作结果。 */
    (void)led_toggle(blue);
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(LED_PERIOD_MS));
  }
}
