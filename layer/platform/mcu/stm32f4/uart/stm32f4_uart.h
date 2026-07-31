/**
 * @file    stm32f4_uart.h
 * @brief   STM32F4 UART 具体实现的板级绑定接口。
 * @author  weixin
 */
#ifndef STM32F4_UART_H
#define STM32F4_UART_H

#include "platform/inferface/uart/uart_blocking.h"
#include "platform/inferface/uart/uart_interrupt.h"

#include <stdint.h>

struct stm32f4_uart_config {
  int32_t id;
  const char *name;
  /* Board 选择实际接线；后端负责校验该引脚是否支持对应 UART 信号。 */
  int32_t tx_pin;
  int32_t rx_pin;
  uint32_t baud_rate;
  uint32_t timeout_ms;
  uint32_t irq_priority;
};

/**
 * @brief  创建带阻塞接收能力的 STM32F4 UART 对象。
 * @param  config Board 层传入的 UART 控制器、引脚和波特率配置。
 * @return UART 基础对象；配置无效时返回 NULL。
 * @note   本函数只做对象绑定，不访问硬件；uart_start() 才会真正配置
 *         GPIO、USART、时钟等资源。USART1 当前支持 PA9/PA10 或 PB6/PB7，
 *         TX 与 RX 也可以从各自的合法复用引脚中独立选择。
 */
struct uart *stm32f4_uart_blocking_create(
    const struct stm32f4_uart_config *config);

/**
 * @brief  创建带中断接收能力的 STM32F4 UART 对象。
 * @param  config Board 层传入的 UART 控制器、引脚和波特率配置。
 * @return UART 基础对象；配置无效时返回 NULL。
 * @note   与阻塞实现共用同一个 USART1 硬件资源，所以两者不能同时启动。
 *         未来需要 DMA 接收时，应增加 DMA 能力表和对应具体实现。
 */
struct uart *stm32f4_uart_interrupt_create(
    const struct stm32f4_uart_config *config);

/**
 * @brief  处理 USART1 全局中断。
 * @param  无。
 * @return 无。
 * @note   仅由 USART1_IRQHandler() 在中断上下文调用；应用层不应直接调用。
 */
void stm32f4_usart1_irq_handler(void);

/**
 * @brief  处理 USART3 全局中断。
 * @retval None。
 * @note   仅由 USART3_IRQHandler() 在中断上下文调用。
 */
void stm32f4_usart3_irq_handler(void);

/**
 * @brief  处理 USART6 全局中断。
 * @param  None。
 * @retval None。
 * @note   仅由 USART6_IRQHandler() 在中断上下文调用。
 */
void stm32f4_usart6_irq_handler(void);

/**
 * @brief  处理 USART6 发送使用的 DMA2 Stream6 中断。
 * @param  None。
 * @retval None。
 * @note   仅由 DMA2_Stream6_IRQHandler() 在中断上下文调用。
 */
void stm32f4_usart6_tx_dma_irq_handler(void);

#endif /* STM32F4_UART_H */
