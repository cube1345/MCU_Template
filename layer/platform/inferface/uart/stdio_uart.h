/* SPDX-License-Identifier: MIT */
/**
 * @file    stdio_uart.h
 * @brief   将 C 标准输出绑定到 Platform UART。
 * @author  weixin
 *
 * 本模块不选择具体串口，也不依赖 Board 或 HAL。板级初始化成功后传入抽象
 * struct uart，随后 printf()/fputc() 的字符通过该 UART 发送。
 */
#ifndef STDIO_UART_H
#define STDIO_UART_H

#include "platform/inferface/uart/uart.h"

/**
 * @brief  绑定标准输出使用的 UART 对象。
 * @param  uart 已启动的 UART 对象。
 * @return PLATFORM_OK 表示绑定成功；其他 PLATFORM_* 表示失败。
 * @note   同一时刻只保存一个标准输出 UART。
 */
int stdio_uart_bind(struct uart *uart);

#endif /* STDIO_UART_H */
