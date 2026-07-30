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
  (void)xTask;
  (void)pcTaskName;

  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationMallocFailedHook(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}
