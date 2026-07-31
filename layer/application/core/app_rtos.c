/**
 * @file    app_rtos.c
 * @brief   本工程默认 FreeRTOS 任务的创建与调度器启动。
 * @author  weixin
 *
 * 各任务的业务逻辑分别位于 app_led、app_key、app_oled、app_motor、
 * app_emm_v5、app_tracking、app_ball_control 和 app_serial_monitor 模块。
 * 本文件只维护统一调度配置和启动流程。
 */
#include "app_rtos.h"

#include "app_ball_control.h"
#include "app_emm_v5.h"
#include "app_key.h"
#include "app_led.h"
#include "app_motor.h"
#include "app_mission.h"
#include "app_oled.h"
#include "app_serial_monitor.h"
#include "app_system_stats.h"
#include "app_tracking.h"
#include "app_vision.h"
#include "board_drive.h"
#include "board_tracking.h"
#include "board_uart.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* 栈深度单位为 StackType_t，不是字节。 */
#define LED_STACK_DEPTH (configMINIMAL_STACK_SIZE)
#define KEY_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)
#define OLED_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)
#define MOTOR_STACK_DEPTH (configMINIMAL_STACK_SIZE)
#define EMM_V5_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)
#define SERIAL_MONITOR_STACK_DEPTH (configMINIMAL_STACK_SIZE * 3U)
#define SYSTEM_STATS_STACK_DEPTH (configMINIMAL_STACK_SIZE * 3U)
#define TRACKING_STACK_DEPTH (configMINIMAL_STACK_SIZE)
#define VISION_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)
#define BALL_CONTROL_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)
#define MISSION_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2U)

/* 所有应用任务优先级均基于 FreeRTOS 空闲任务优先级。 */
#define LED_PRIORITY (tskIDLE_PRIORITY + 1U)
#define OLED_PRIORITY (tskIDLE_PRIORITY + 1U)
#define KEY_PRIORITY (tskIDLE_PRIORITY + 2U)
#define MOTOR_PRIORITY (tskIDLE_PRIORITY + 3U)
#define EMM_V5_PRIORITY (tskIDLE_PRIORITY + 1U)
#define SERIAL_MONITOR_PRIORITY (tskIDLE_PRIORITY + 1U)
#define SYSTEM_STATS_PRIORITY (tskIDLE_PRIORITY + 1U)
#define TRACKING_PRIORITY (tskIDLE_PRIORITY + 1U)
#define VISION_PRIORITY (tskIDLE_PRIORITY + 1U)
#define BALL_CONTROL_PRIORITY (tskIDLE_PRIORITY + 2U)
#define MISSION_PRIORITY (tskIDLE_PRIORITY + 2U)

/* 防止 tasks_create() 被重复调用后重复创建任务。 */
static bool s_tasks_created;

/**
 * @brief  初始化任务依赖并创建工程默认应用任务。
 * @return PLATFORM_OK 表示任务创建成功或已经创建。
 * @return 其他 PLATFORM_* 表示依赖初始化或任务创建失败。
 * @note   使用 FreeRTOS 动态创建任务，不在本模块持有任务句柄。
 */
int tasks_create(void)
{
  int result;

  if (s_tasks_created) {
    return PLATFORM_OK;
  }

  result = board_uart1_init();
  if (result != PLATFORM_OK) {
    return result;
  }
  result = board_drive_bind();
  if (result != PLATFORM_OK) {
    return result;
  }
  result = board_tracking_init();
  if (result != PLATFORM_OK) {
    return result;
  }
  result = mission_init();
  if (result != PLATFORM_OK) {
    return result;
  }

  if (xTaskCreate(serial_monitor_task, "monitor",
                  SERIAL_MONITOR_STACK_DEPTH, NULL,
                  SERIAL_MONITOR_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(system_stats_task, "sys_stats",
                  SYSTEM_STATS_STACK_DEPTH, NULL,
                  SYSTEM_STATS_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(led_task, "led", LED_STACK_DEPTH,
                  NULL, LED_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(key_task, "key", KEY_STACK_DEPTH,
                  NULL, KEY_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(mission_task, "mission", MISSION_STACK_DEPTH,
                  NULL, MISSION_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(oled_task, "oled", OLED_STACK_DEPTH,
                  NULL, OLED_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(motor_task, "motor", MOTOR_STACK_DEPTH,
                  NULL, MOTOR_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(emm_v5_task, "emm_v5",
                  EMM_V5_STACK_DEPTH, NULL,
                  EMM_V5_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(tracking_task, "tracking",
                  TRACKING_STACK_DEPTH, NULL,
                  TRACKING_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(vision_task, "vision",
                  VISION_STACK_DEPTH, NULL,
                  VISION_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }
  if (xTaskCreate(ball_control_task, "ball_ctrl",
                  BALL_CONTROL_STACK_DEPTH, NULL,
                  BALL_CONTROL_PRIORITY, NULL) != pdPASS) {
    return PLATFORM_ERR_IO;
  }

  s_tasks_created = true;
  printf("[app] tasks created\r\n");
  return PLATFORM_OK;
}

/**
 * @brief  启动 FreeRTOS 调度器。
 * @return 正常情况下不返回；返回时表示调度器启动失败。
 * @note   必须在 tasks_create() 成功之后调用。
 */
int rtos_start(void)
{
  if (!s_tasks_created) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  printf("[app] starting FreeRTOS scheduler\r\n");
  vTaskStartScheduler();

  /* 正常启动后不会返回；返回通常表示内核启动资源不足。 */
  return PLATFORM_ERR_INVALID_STATE;
}
