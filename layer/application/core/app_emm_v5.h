/* SPDX-License-Identifier: MIT */
/**
 * @file    app_emm_v5.h
 * @brief   EMM V5闭环步进电机使能和位置监测任务入口。
 * @author  weixin
 */
#ifndef APP_EMM_V5_H
#define APP_EMM_V5_H

#include <stdbool.h>

/**
 * @brief  发布EMM绝对位置目标，由EMM任务通过USART6串行下发。
 * @param  position_deg 相对启动零点的绝对角度，单位为度。
 * @retval PLATFORM_OK表示目标已写入邮箱，其他值表示参数非法。
 * @note   本函数不访问硬件，可由普通FreeRTOS任务调用，禁止在ISR调用。
 */
int emm_v5_target_position_deg_set(float position_deg);

/**
 * @brief  请求EMM立即停止并暂停处理球控绝对位置目标。
 * @param  hold true表示停止并保持，false表示恢复执行最新位置目标。
 * @retval PLATFORM_OK表示请求已写入邮箱。
 * @note   本函数不访问硬件，实际停止命令由EMM任务通过USART6发送。
 */
int emm_v5_hold_set(bool hold);

/**
 * @brief  使能EMM V5步进电机并周期读取实时位置。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None，任务启动后持续运行，发生错误时主动删除自身。
 * @note   本任务独占EMM和USART6，并执行球控或WEIXIN绝对位置目标。
 */
void emm_v5_task(void *argument);

#endif /* APP_EMM_V5_H */
