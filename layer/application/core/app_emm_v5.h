/* SPDX-License-Identifier: MIT */
/**
 * @file    app_emm_v5.h
 * @brief   EMM V5闭环步进电机使能和位置监测任务入口。
 * @author  weixin
 */
#ifndef APP_EMM_V5_H
#define APP_EMM_V5_H

/**
 * @brief  使能EMM V5步进电机并周期读取实时位置。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None，任务启动后持续运行，发生错误时主动删除自身。
 * @note   本任务不会发送速度或位置运动命令。
 */
void emm_v5_task(void *argument);

#endif /* APP_EMM_V5_H */
