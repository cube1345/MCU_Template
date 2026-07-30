/**
 * @file    uart.h
 * @brief   平台 UART 抽象父类，负责公共生命周期和发送能力。
 * @author  weixin
 *
 * 本文件描述“一个串口”的基础能力和固定扩展能力，不暴露 HAL 句柄、
 * 寄存器或具体引脚。应用层始终持有 struct uart *，通过能力包装函数
 * 使用阻塞接收、中断接收等扩展能力。
 */
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

struct uart;
struct uart_blocking_ops;
struct uart_interrupt_ops;

struct uart_ops {
  /* 启动具体子类拥有的时钟、GPIO 复用、外设和中断资源。 */
  int (*start)(struct uart *self);
  /* 释放 start() 获取的资源；存在未完成中断接收时必须返回忙。 */
  int (*stop)(struct uart *self);
  /*
   * 发送是 UART 的公共能力，所以保留在父类动作表。
   * 接收方式差异较大，放到阻塞/中断/DMA 子类自己的动作表中。
   */
  int (*write)(struct uart *self, const uint8_t *data,
               uint16_t size, uint32_t timeout_ms);
};

struct uart {
  /* 启动前由具体子类绑定，启动后保持不变。 */
  const struct uart_ops *ops;
  /* 不支持的固定能力绑定为 NULL，包装函数返回 UNSUPPORTED。 */
  const struct uart_blocking_ops *blocking_ops;
  const struct uart_interrupt_ops *interrupt_ops;
  /* 调试名称，生命周期由板级静态配置保证。 */
  const char *name;
  /* 仅由 uart_start/stop() 维护，调用者不要直接修改。 */
  bool initialized;
};

/**
 * @brief  启动具体 UART 子类持有的全部硬件资源。
 * @param  self 已由具体子类绑定 ops 和名称的基类对象。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 * @note   仅允许任务上下文调用。
 */
int uart_start(struct uart *self);

/**
 * @brief  停止 UART 并释放外设、引脚、时钟和中断资源。
 * @param  self 已启动的 UART 对象。
 * @return PLATFORM_OK 表示释放成功；接收未完成时返回 PLATFORM_ERR_BUSY。
 */
int uart_stop(struct uart *self);

/**
 * @brief  阻塞发送固定长度数据。
 * @param  self 已启动的 UART 对象。
 * @param  data 调用返回前保持有效的只读缓冲区。
 * @param  size 非零发送长度。
 * @param  timeout_ms 等待超时，允许使用 PLATFORM_WAIT_FOREVER。
 * @return PLATFORM_OK 表示发送成功；其他 PLATFORM_* 表示失败。
 */
int uart_write(struct uart *self, const uint8_t *data,
               uint16_t size, uint32_t timeout_ms);

#endif /* UART_H */
