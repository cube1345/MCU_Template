/* SPDX-License-Identifier: MIT */
/**
 * @file    board_emm_v5.h
 * @brief   STM32F407板级EMM V5闭环步进电机访问接口。
 * @author  weixin
 */
#ifndef BOARD_EMM_V5_H
#define BOARD_EMM_V5_H

#include "device/motor/motor.h"

/**
 * @brief  绑定板载EMM V5电机对象和其USART6通信资源。
 * @retval PLATFORM_OK表示绑定成功或已经绑定，其他PLATFORM_*表示失败。
 * @note   电机使用USART6的PC6(TX)/PC7(RX)。本函数启动UART但不发送运动命令。
 */
int board_emm_v5_init(void);

/**
 * @brief  获取板载EMM V5对应的motor基础对象。
 * @retval board_emm_v5_init()成功后返回motor对象，否则返回NULL。
 */
struct motor *board_emm_v5_get(void);

#endif /* BOARD_EMM_V5_H */
