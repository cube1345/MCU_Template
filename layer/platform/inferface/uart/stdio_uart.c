/* SPDX-License-Identifier: MIT */
/**
 * @file    stdio_uart.c
 * @brief   printf/fputc 到 Platform UART 的标准输出重定向。
 * @author  weixin
 *
 * printf 可能逐字符调用 fputc，因此本实现适合调试日志，不适合高吞吐数据传输。
 * 必须在任务上下文使用；多个任务同时打印时，调用者需要在更高层做串行化。
 */
#include "platform/inferface/uart/stdio_uart.h"

#include "result.h"

#include <stddef.h>
#include <stdio.h>

#define STDIO_UART_TIMEOUT_MS UINT32_C(1000)

static struct uart *g_stdio_uart;

/**
 * @brief  绑定标准输出使用的 UART 对象。
 * @param  uart 已启动的 UART 对象。
 * @return PLATFORM_OK 表示绑定成功；其他 PLATFORM_* 表示失败。
 * @note   同一时刻只保存一个标准输出 UART。
 */
int stdio_uart_bind(struct uart *uart)
{
  if ((uart == NULL) || !uart->initialized) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  g_stdio_uart = uart;
  return PLATFORM_OK;
}

/*
 * ARMCC 标准库默认使用半主机。关闭半主机后提供最小退出/终端输出符号，
 * 防止脱离调试器运行时因半主机调用进入异常。
 */
#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)

struct __FILE {
  int handle;
};

FILE __stdout;

/**
 * @brief  ARMCC 无半主机模式下的最小退出桩函数。
 * @param  return_code 退出码，当前未使用。
 * @return 无返回值。
 */
void _sys_exit(int return_code)
{
  (void)return_code;
  for (;;) {
  }
}
#endif

/**
 * @brief  将 C 标准库字符输出重定向到已绑定 UART。
 * @param  character 待输出字符。
 * @param  stream 标准库文件流，当前未使用。
 * @return 成功时返回输出字符，失败时返回 EOF。
 */
int fputc(int character, FILE *stream)
{
  uint8_t byte;

  (void)stream;
  if (g_stdio_uart == NULL) {
    return EOF;
  }

  byte = (uint8_t)character;
  if (uart_write(g_stdio_uart, &byte, UINT16_C(1),
                 STDIO_UART_TIMEOUT_MS) != PLATFORM_OK) {
    return EOF;
  }
  return character;
}

#if defined(__CC_ARM)
/**
 * @brief  ARMCC 终端单字符输出钩子。
 * @param  character 待输出字符。
 * @return 无返回值。
 */
void _ttywrch(int character)
{
  (void)fputc(character, &__stdout);
}
#endif
