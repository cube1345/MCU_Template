/**
 * @file    app_tracking.h
 * @brief   八路循迹差速控制任务入口。
 * @author  weixin
 */
#ifndef APP_TRACKING_H
#define APP_TRACKING_H

/**
 * @brief  周期采样八路循迹输入并发布左右轮目标速度。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @retval None。
 * @note   任务不访问电机硬件，左右闭环仍由电机任务独占执行。
 */
void tracking_task(void *argument);

#endif /* APP_TRACKING_H */
