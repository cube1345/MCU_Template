/**
 * @file    uart_blocking.c
 * @brief   阻塞接收 UART 子类的包装接口。
 * @author  weixin
 */
#include "platform/inferface/uart/uart_blocking.h"

#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 UART 阻塞能力是否可用。
 * @param  self UART 基类对象。
 * @return true 表示对象已启动，false 表示不可用。
 */
static bool uart_blocking_is_ready(const struct uart *self)
{
  return (self != NULL) && self->initialized;
}

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
                       uint32_t timeout_ms)
{
  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!uart_blocking_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((self->blocking_ops == NULL) ||
      (self->blocking_ops->read == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  if ((data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  return self->blocking_ops->read(self, data, size, timeout_ms);
}
