/**
 * @file    app_key.c
 * @brief   用户按键事件轮询应用。
 * @author  weixin
 */
#include "app_key.h"
#include "app_ball_control.h"
#include "app_emm_v5.h"
#include "app_mission.h"

#include "board_key.h"
#include "key_event.h"
#include "result.h"
#include "stm32f4.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 用户按键事件状态机的轮询周期，单位为毫秒。 */
#define KEY_PERIOD_MS UINT32_C(10)

/* 用户按键任务独占的按键事件状态机对象。 */
static struct key_event s_user_key_event;
static struct key_event s_emm_enable_key_event;
static struct key_event s_reset_key_event;

/**
 * @brief  运行用户按键事件轮询任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务负责初始化并独占用户按键及其事件状态机。
 */
void key_task(void *argument)
{
  struct key *user_key;
  struct key *emm_enable_key;
  struct key *reset_key;
  TickType_t last_wake_time;
  bool emm_enabled = true;
  int result;
  int event;
  int emm_event;
  int reset_event;

  (void)argument;

  result = board_key_init();
  if (result != PLATFORM_OK) {
    printf("[key] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  user_key = board_key_user_get();
  emm_enable_key = board_key_emm_enable_get();
  reset_key = board_key_reset_get();
  if ((user_key == NULL) || (emm_enable_key == NULL) ||
      (reset_key == NULL)) {
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
  result = key_event_init(&s_reset_key_event, reset_key);
  if (result != PLATFORM_OK) {
    printf("[key] reset event init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }
  result = key_event_init(&s_emm_enable_key_event,
                          emm_enable_key);
  if (result != PLATFORM_OK) {
    printf("[key] emm event init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  for (;;) {
    /* Only release events are forwarded; hold/repeat events stay local. */
    event = key_event_poll(&s_user_key_event);
    if (event == KEY_EVENT_SHORT_RELEASE) {
      result = mission_key_event_post(MISSION_KEY_SHORT_RELEASE);
    } else if (event == KEY_EVENT_LONG_RELEASE) {
      result = mission_key_event_post(MISSION_KEY_LONG_RELEASE);
    } else {
      result = (event < PLATFORM_OK) ? event : PLATFORM_OK;
    }
    if (result != PLATFORM_OK) {
      printf("[key] event publish failed: %d\r\n", result);
    }
    emm_event = key_event_poll(&s_emm_enable_key_event);
    if (emm_event == KEY_EVENT_SHORT_RELEASE) {
      emm_enabled = !emm_enabled;
      result = emm_v5_enabled_request_set(emm_enabled);
      if ((result == PLATFORM_OK) && !emm_enabled) {
        result = ball_control_target_x_set(INT16_C(0));
      }
      if ((result == PLATFORM_OK) && !emm_enabled) {
        result = ball_control_enabled_set(false);
      }
      if (result == PLATFORM_OK) {
        printf("[key] emm enabled=%u requested\r\n",
               emm_enabled ? 1U : 0U);
      }
    } else {
      result = (emm_event < PLATFORM_OK) ?
               emm_event : PLATFORM_OK;
    }
    if (result != PLATFORM_OK) {
      printf("[key] emm event failed: %d\r\n", result);
    }
    reset_event = key_event_poll(&s_reset_key_event);
    if (reset_event == KEY_EVENT_SHORT_RELEASE) {
      printf("[key] system reset requested\r\n");
      vTaskDelay(pdMS_TO_TICKS(UINT32_C(20)));
      stm32f4_system_reset();
    } else if (reset_event < PLATFORM_OK) {
      printf("[key] reset event failed: %d\r\n", reset_event);
    }
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(KEY_PERIOD_MS));
  }
}
