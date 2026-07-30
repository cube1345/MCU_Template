/**
 * @file    uart_blocking.h
 * @brief   平台阻塞接收 UART 子类。
 * @author  weixin
 *
 * 这是 UART 的固定阻塞接收能力表，不是应用层可见的具体子类。
 */
#ifndef UART_BLOCKING_H
#define UART_BLOCKING_H

#include "platform/inferface/uart/uart.h"

struct uart_blocking_ops {
  /* 阻塞接收固定长度数据；不能在 ISR 上下文调用。 */
  int (*read)(struct uart *self, uint8_t *data,
              uint16_t size, uint32_t timeout_ms);
};

/**
 * @brief  阻塞接收固定长度数据。
 * @param  self 已启动的 UART 对象。
 * @param  data 接收缓冲区，调用期间必须保持有效。
 * @param  size 非零接收长度。
 * @param  timeout_ms 等待超时时间，单位 ms。
 * @return PLATFORM_OK 表示接收成功；其他 PLATFORM_* 表示失败。
 */
int uart_blocking_read(struct uart *self,
                       uint8_t *data, uint16_t size,
                       uint32_t timeout_ms);

#endif /* UART_BLOCKING_H */
