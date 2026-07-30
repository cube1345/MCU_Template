/**
 * @file    app_system_stats.c
 * @brief   FreeRTOS 系统状态按需报告任务实现。
 * @author  weixin
 */
#include "app_system_stats.h"

#include "my_freetros_lib.h"

#include "FreeRTOS.h"
#include "task.h"

/* 系统状态任务句柄，仅用于让串口命令唤醒该任务并立即生成报告。 */
static TaskHandle_t s_system_stats_task_handle;

/**
 * @brief  请求系统状态任务立即打印一次完整报告。
 * @retval None
 * @note   只能从任务上下文调用；任务尚未开始运行时请求会被忽略。
 */
void system_stats_report_request(void)
{
  TaskHandle_t task_handle;

  /* 句柄在任务首次运行时写入，短临界区保证读取过程不被切换打断。 */
  taskENTER_CRITICAL();
  task_handle = s_system_stats_task_handle;
  taskEXIT_CRITICAL();

  if (task_handle != NULL) {
    xTaskNotifyGive(task_handle);
  }
}

/**
 * @brief  等待报告请求并打印任务状态、CPU 占用率和堆内存统计信息。
 * @param  argument：FreeRTOS 任务参数，当前实现不使用。
 * @retval None
 * @note   每收到一个任务通知打印一次，未收到命令时不会主动输出。
 */
void system_stats_task(void *argument)
{
  (void)argument;

  taskENTER_CRITICAL();
  s_system_stats_task_handle = xTaskGetCurrentTaskHandle();
  taskEXIT_CRITICAL();

  for (;;) {
    /* pdFALSE 每次只消费一个通知，保证每条命令对应一份报告。 */
    (void)ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    Show_System_Stats();
  }
}
