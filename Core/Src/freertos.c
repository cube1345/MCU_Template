/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : freertos.c
  * @brief          : FreeRTOS application integration.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "rtos_app.h"

#include "board_led.h"
#include "main.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdio.h>

/* Kept in RAM so a debugger can inspect the failure even if UART output fails. */
volatile TaskHandle_t g_freertos_failed_task;
const char * volatile g_freertos_failed_task_name;
volatile size_t g_freertos_failed_free_heap;
volatile size_t g_freertos_failed_minimum_heap;

void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  if (board_led_init() != PLATFORM_OK)
  {
    Error_Handler();
  }

  /* Create application tasks, queues, mutexes, and timers here before
   * vTaskStartScheduler() is called from main.c. */
  /* USER CODE END Init */
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  g_freertos_failed_task = xTask;
  g_freertos_failed_task_name = pcTaskName;

  printf("[fatal] FreeRTOS stack overflow: task=%s\r\n",
         (pcTaskName != NULL) ? pcTaskName : "<unknown>");

  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationMallocFailedHook(void)
{
  g_freertos_failed_free_heap = xPortGetFreeHeapSize();
  g_freertos_failed_minimum_heap = xPortGetMinimumEverFreeHeapSize();

  printf("[fatal] FreeRTOS malloc failed: free=%lu min_ever=%lu bytes\r\n",
         (unsigned long)g_freertos_failed_free_heap,
         (unsigned long)g_freertos_failed_minimum_heap);

  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}
