/* SPDX-License-Identifier: MIT */
/**
 * @file    app_serial_monitor.h
 * @brief   WEIXIN Serial Monitor UART1 通信任务入口。
 * @author  weixin
 */
#ifndef APP_SERIAL_MONITOR_H
#define APP_SERIAL_MONITOR_H

#include <stdint.h>

/*
 * 速度环串口调试总开关：1 开启调参和波形，0 关闭并裁剪相关代码。
 * 允许由编译选项预定义该宏覆盖默认值。
 */
#ifndef SERIAL_MONITOR_DRIVE_DEBUG_ENABLE
#define SERIAL_MONITOR_DRIVE_DEBUG_ENABLE 0
#endif

#if ((SERIAL_MONITOR_DRIVE_DEBUG_ENABLE != 0) && \
     (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE != 1))
#error "SERIAL_MONITOR_DRIVE_DEBUG_ENABLE must be 0 or 1"
#endif

/**
 * @brief  获取上位机请求的电机速度 PI 参数快照。
 * @param  speed_kp 用于接收比例增益的变量。
 * @param  speed_ki 用于接收积分增益的变量。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   本函数只读取待应用值，不直接修改闭环控制器。
 */
int serial_monitor_drive_speed_gains_get(float *speed_kp,
                                         float *speed_ki);

/**
 * @brief  获取上位机配置的左右轮循迹基础速度快照。
 * @param  left_speed 用于接收左轮基础速度，单位为编码器计数每秒。
 * @param  right_speed 用于接收右轮基础速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   循迹任务读取配置并叠加差速，本函数不发布电机目标。
 */
int serial_monitor_drive_base_speeds_get(int32_t *left_speed,
                                         int32_t *right_speed);

#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
/**
 * @brief  发布左右轮实测速度，供 WEIXIN 波形协议低频发送。
 * @param  left_speed 左轮实测速度，单位为编码器计数每秒。
 * @param  right_speed 右轮实测速度，单位为编码器计数每秒。
 * @retval None。
 * @note   本函数只更新内存快照，不会在电机控制任务中执行串口发送。
 */
void serial_monitor_drive_speeds_publish(int32_t left_speed,
                                         int32_t right_speed);
#endif

/**
 * @brief  运行 WEIXIN Serial Monitor UART1 通信任务。
 * @param  argument：FreeRTOS 任务参数，当前未使用。
 * @retval None。
 * @note   USART1 ISR 只投递字节事件；本任务阻塞等待 FreeRTOS 队列并在任务上下文解析协议。
 */
void serial_monitor_task(void *argument);

#endif /* APP_SERIAL_MONITOR_H */
