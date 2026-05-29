#ifndef __HAL_UART_H
#define __HAL_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#if defined(HAL_UART_MODULE_ENABLED) && defined(__has_include)
#if __has_include("stm32f4xx_hal_uart.h")
#include "stm32f4xx_hal_uart.h"
#define HAL_UART_STM32_HAS_UART        (1U)
#endif
#endif

#ifndef HAL_UART_STM32_HAS_UART
#define HAL_UART_STM32_HAS_UART        (0U)
#endif

#if (HAL_UART_STM32_HAS_UART == 0U)
typedef void UART_HandleTypeDef;
#endif

typedef UART_HandleTypeDef *platform_uart_regs_t;
typedef IRQn_Type platform_uart_irqn_t;

typedef struct {
    platform_uart_regs_t regs;
    platform_uart_irqn_t int_irqn;
} platform_uart_t;

/* 过程式接口：直接传入 STM32 UART 句柄 */
uint32_t HAL_UART_GetPendingInterrupt(UART_HandleTypeDef *uart);
bool HAL_UART_IsRXFIFOEmpty(UART_HandleTypeDef *uart);
uint8_t HAL_UART_ReceiveData(UART_HandleTypeDef *uart);
bool HAL_UART_TransmitDataCheck(UART_HandleTypeDef *uart, uint8_t data);
uint32_t HAL_UART_FillTXFIFO(UART_HandleTypeDef *uart, const uint8_t *data, uint32_t length);
bool HAL_UART_IsBusy(UART_HandleTypeDef *uart);

/* 对象式接口：传入 platform_uart_t 指针，带 NULL 检查 */
uint32_t Platform_UART_GetPendingInterrupt(const platform_uart_t *uart);
bool Platform_UART_IsRXFIFOEmpty(const platform_uart_t *uart);
uint8_t Platform_UART_ReceiveData(const platform_uart_t *uart);
bool Platform_UART_TransmitDataCheck(const platform_uart_t *uart, uint8_t data);
uint32_t Platform_UART_FillTXFIFO(const platform_uart_t *uart, const uint8_t *data, uint32_t length);
bool Platform_UART_IsBusy(const platform_uart_t *uart);

#endif
