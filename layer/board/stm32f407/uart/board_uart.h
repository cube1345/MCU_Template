/**
 * @file    board_uart.h
 * @brief   STM32F407 开发板 UART 的板级访问接口。
 * @author  weixin
 *
 * Board 层只描述“这块板子的 USART1 接到哪里、默认用什么参数”。真正的
 * UART 能力由 uart_blocking 或 uart_interrupt 子类提供。
 */
#ifndef BOARD_UART_H
#define BOARD_UART_H

#include "platform/inferface/uart/uart.h"

/**
 * @brief  初始化 STM32F407 板级 USART1。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU、UART 或 printf 重定向初始化失败。
 */
int board_uart1_init(void);

/**
 * @brief  获取板级 USART1 抽象 UART 对象。
 * @return 初始化成功后返回 struct uart *，否则返回 NULL。
 */
struct uart *board_uart1_get(void);

/**
 * @brief  初始化连接EMM V5驱动器的STM32F407板级USART6。
 * @retval PLATFORM_OK表示初始化成功或已经初始化，其他PLATFORM_*表示失败。
 * @note   USART6固定使用PC6作为TX、PC7作为RX，默认波特率为115200。
 */
int board_uart6_init(void);

/**
 * @brief  获取EMM V5连接使用的板级USART6抽象对象。
 * @retval 初始化成功后返回struct uart *，否则返回NULL。
 */
struct uart *board_uart6_get(void);

#endif /* BOARD_UART_H */
