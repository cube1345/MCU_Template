/**
 * @file    uart_interrupt.c
 * @brief   中断接收 UART 子类的包装接口。
 * @author  weixin
 */
#include "platform/inferface/uart/uart_interrupt.h"

#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 UART 中断接收能力是否可用。
 * @param  self UART 基类对象。
 * @return true 表示对象已启动，false 表示不可用。
 */
static bool uart_interrupt_is_ready(const struct uart *self)
{
  return (self != NULL) && self->initialized;
}

/**
 * @brief  启动一次 UART 中断接收。
 * @param  self 已启动的 UART 对象。
 * @param  data 接收缓冲区，在回调触发前必须保持有效。
 * @param  size 非零接收长度。
 * @param  callback 接收完成或错误回调。
 * @param  context 调用者上下文指针，平台层不解释。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
int uart_interrupt_receive(
    struct uart *self,
    uint8_t *data,
    uint16_t size,
    uart_interrupt_callback_t callback,
    void *context)
{
  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!uart_interrupt_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((self->interrupt_ops == NULL) ||
      (self->interrupt_ops->receive == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  if ((data == NULL) || (size == 0U) || (callback == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  return self->interrupt_ops->receive(self, data, size, callback, context);
}
