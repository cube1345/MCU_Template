/**
 * @file    app_system_stats.h
 * @brief   FreeRTOS 系统状态按需报告任务接口。
 * @author  weixin
 */
#ifndef APP_SYSTEM_STATS_H
#define APP_SYSTEM_STATS_H

/**
 * @brief  请求系统状态任务立即打印一次完整报告。
 * @retval None
 * @note   只能从任务上下文调用；每次请求对应一次完整报告。
 */
void system_stats_report_request(void);

/**
 * @brief  等待报告请求并打印任务状态、CPU 占用率和堆内存统计信息。
 * @param  argument：FreeRTOS 任务参数，当前实现不使用。
 * @retval None
 * @note   该函数是 FreeRTOS 任务入口，创建后不会返回。
 */
void system_stats_task(void *argument);

#endif /* APP_SYSTEM_STATS_H */
