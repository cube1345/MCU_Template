/* SPDX-License-Identifier: MIT */
/**
 * @file    app_emm_v5.h
 * @brief   EMM V5闭环步进电机恒速任务入口。
 * @author  weixin
 */
#ifndef APP_EMM_V5_H
#define APP_EMM_V5_H

/**
 * @brief  使能EMM V5步进电机并发送一次恒速命令。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None，任务启动后持续运行，发生错误时主动删除自身。
 * @note   恒速命令发送成功后任务仅保持存活，不再重复发送调速命令。
 */
void emm_v5_task(void *argument);

#endif /* APP_EMM_V5_H */
