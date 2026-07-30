/**
 * @file    app_key.c
 * @brief   用户按键事件轮询应用。
 * @author  weixin
 */
#include "app_key.h"

#include "board_key.h"
#include "key_event.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 用户按键事件状态机的轮询周期，单位为毫秒。 */
#define KEY_PERIOD_MS UINT32_C(10)

/* 用户按键任务独占的按键事件状态机对象。 */
static struct key_event s_user_key_event;

/**
 * @brief  运行用户按键事件轮询任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务负责初始化并独占用户按键及其事件状态机。
 */
void key_task(void *argument)
{
  struct key *user_key;
  TickType_t last_wake_time;
  int result;

  (void)argument;

  result = board_key_init();
  if (result != PLATFORM_OK) {
    printf("[key] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  user_key = board_key_user_get();
  if (user_key == NULL) {
    printf("[key] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  result = key_event_init(&s_user_key_event, user_key);
  if (result != PLATFORM_OK) {
    printf("[key] event init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  for (;;) {
    /* 当前示例不消费按键事件，不保存事件统计或错误状态。 */
    (void)key_event_poll(&s_user_key_event);
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(KEY_PERIOD_MS));
  }
}
