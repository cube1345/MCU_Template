/**
 * @file    uart.c
 * @brief   平台 UART 抽象基类的状态检查和操作分发。
 * @author  weixin
 */
#include "platform/inferface/uart/uart.h"

#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 UART 基类对象是否已经启动。
 * @param  self UART 基类对象。
 * @return true 表示对象可用，false 表示对象无效或未启动。
 */
static bool uart_is_ready(const struct uart *self)
{
  /* 所有公开操作先统一做状态门禁，避免空对象进入具体 MCU 后端。 */
  return (self != NULL) && self->initialized && (self->ops != NULL);
}

/**
 * @brief  启动具体 UART 子类持有的硬件资源。
 * @param  self 已绑定 ops 和名称的 UART 基类对象。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
int uart_start(struct uart *self)
{
  int result;

  if ((self == NULL) || (self->ops == NULL) ||
      (self->ops->start == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  /*
   * 硬件启动交给具体子类。只有子类成功拿到全部资源后，基类才标记
   * initialized，避免半启动对象被后续收发接口使用。
   */
  result = self->ops->start(self);
  if (result == PLATFORM_OK) {
    self->initialized = true;
  }
  return result;
}

/**
 * @brief  停止 UART 并释放外设、引脚、时钟和中断资源。
 * @param  self 已启动的 UART 对象。
 * @return PLATFORM_OK 表示释放成功；其他 PLATFORM_* 表示失败。
 */
int uart_stop(struct uart *self)
{
  int result;

  if (!uart_is_ready(self) || (self->ops->stop == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  /* 停止失败时保持 initialized=true，调用者可重试或继续使用对象。 */
  result = self->ops->stop(self);
  if (result == PLATFORM_OK) {
    self->initialized = false;
  }
  return result;
}

/**
 * @brief  阻塞发送固定长度数据。
 * @param  self 已启动的 UART 对象。
 * @param  data 调用返回前保持有效的只读缓冲区。
 * @param  size 非零发送长度。
 * @param  timeout_ms 等待超时时间，单位 ms。
 * @return PLATFORM_OK 表示发送成功；其他 PLATFORM_* 表示失败。
 */
int uart_write(struct uart *self, const uint8_t *data,
               uint16_t size, uint32_t timeout_ms)
{
  if (!uart_is_ready(self) || (self->ops->write == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  /* 基类不复制缓冲区；调用期间 data 必须由调用者保持有效。 */
  return self->ops->write(self, data, size, timeout_ms);
}
