/**
 * @file    my_freetros_lib.h
 * @brief   FreeRTOS 运行状态监控工具接口。
 * @author  weixin
 */
#ifndef MY_FREETROS_LIB_H
#define MY_FREETROS_LIB_H

/**
 * @brief  打印 FreeRTOS 任务运行状态、CPU 占用率和堆内存统计信息。
 * @retval None
 * @note   Avg% 为系统启动以来的累计平均 CPU 占用率。
 * @note   Win% 为本次调用与上次调用之间的窗口 CPU 占用率。
 * @note   只能从任务上下文调用，本函数不可重入。
 */
void Show_System_Stats(void);

#endif /* MY_FREETROS_LIB_H */
