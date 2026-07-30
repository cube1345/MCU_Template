/**
 * @file    uart_interrupt.h
 * @brief   平台中断接收 UART 子类。
 * @author  weixin
 *
 * 这是 UART 的固定中断接收能力表，不是应用层可见的具体子类。
 */
#ifndef UART_INTERRUPT_H
#define UART_INTERRUPT_H

#include "platform/inferface/uart/uart.h"

/*
 * 中断接收完成回调。
 * self    : 触发回调的基础 UART 对象。
 * data    : 调用 receive() 时传入的接收缓冲区。
 * size    : 本次请求接收的字节数；当前 STM32F4 后端按固定长度完成。
 * result  : PLATFORM_OK 表示完成，错误码表示 HAL 或硬件异常。
 * context : 调用者上下文，平台层不解释、不保存所有权。
 *
 * 注意：该回调运行在 USART IRQ 中断上下文，不能阻塞、等待互斥锁或调用
 * HAL_Delay。允许通过 uart_interrupt_receive() 立即续接同一个 UART 的下一次
 * 接收；续接前平台后端已经清除上一笔请求的挂起状态。
 */
typedef void (*uart_interrupt_callback_t)(
    struct uart *self,
    uint8_t *data,
    uint16_t size,
    int result,
    void *context);

struct uart_interrupt_ops {
  /* 启动一次中断接收；一个 UART 同时只允许一个 pending 接收。 */
  int (*receive)(struct uart *self, uint8_t *data,
                 uint16_t size,
                 uart_interrupt_callback_t callback,
                 void *context);
};

/**
 * @brief  启动一次 UART 中断接收。
 * @param  self 已启动的 UART 对象。
 * @param  data 接收缓冲区，在回调触发前必须保持有效。
 * @param  size 非零接收长度。
 * @param  callback 接收完成或错误回调。
 * @param  context 调用者上下文指针，平台层不解释。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 * @note   通常从任务上下文调用；也允许在本 UART 的接收完成回调中续接接收。
 */
int uart_interrupt_receive(
    struct uart *self,
    uint8_t *data,
    uint16_t size,
    uart_interrupt_callback_t callback,
    void *context);

#endif /* UART_INTERRUPT_H */
