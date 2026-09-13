/**
 * @file hal_uart.c
 * @brief STM32U5 byte FIFO operations with explicit UART objects.
 * @details CubeMX/Board configures UART; unavailable bindings never access registers.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#include "hal_uart.h"

static bool hal_uart_available(UART_HandleTypeDef *uart)
{
#if (HAL_UART_STM32_HAS_UART == 1U)
    return (uart != 0) && (uart->Instance != 0) &&
           (uart->gState != HAL_UART_STATE_RESET);
#else
    (void)uart;
    return false;
#endif
}

uint32_t HAL_UART_GetPendingInterrupt(UART_HandleTypeDef *uart)
{
    uint32_t pending = 0U;
    if (!hal_uart_available(uart)) return 0U;
#if (HAL_UART_STM32_HAS_UART == 1U)
    if ((__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) != RESET) &&
        (__HAL_UART_GET_IT_SOURCE(uart, UART_IT_RXNE) != RESET)) {
        pending |= UART_FLAG_RXNE;
    }
    if ((__HAL_UART_GET_FLAG(uart, UART_FLAG_TXE) != RESET) &&
        (__HAL_UART_GET_IT_SOURCE(uart, UART_IT_TXE) != RESET)) {
        pending |= UART_FLAG_TXE;
    }
    if ((__HAL_UART_GET_FLAG(uart, UART_FLAG_TC) != RESET) &&
        (__HAL_UART_GET_IT_SOURCE(uart, UART_IT_TC) != RESET)) {
        pending |= UART_FLAG_TC;
    }
    if ((__HAL_UART_GET_FLAG(uart, UART_FLAG_ORE) != RESET) &&
        ((__HAL_UART_GET_IT_SOURCE(uart, UART_IT_ERR) != RESET) ||
         (__HAL_UART_GET_IT_SOURCE(uart, UART_IT_RXNE) != RESET))) {
        pending |= UART_FLAG_ORE;
    }
#endif
    return pending;
}

bool HAL_UART_IsRXFIFOEmpty(UART_HandleTypeDef *uart)
{
    if (!hal_uart_available(uart)) return true;
#if (HAL_UART_STM32_HAS_UART == 1U)
    return (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) == RESET);
#else
    return true;
#endif
}

uint8_t HAL_UART_ReceiveData(UART_HandleTypeDef *uart)
{
    if (HAL_UART_IsRXFIFOEmpty(uart)) return 0U;
#if (HAL_UART_STM32_HAS_UART == 1U)
    return (uint8_t)(uart->Instance->RDR & 0xFFU);
#else
    return 0U;
#endif
}

bool HAL_UART_TransmitDataCheck(UART_HandleTypeDef *uart, uint8_t data)
{
    if (!hal_uart_available(uart)) return false;
#if (HAL_UART_STM32_HAS_UART == 1U)
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_TXE) == RESET) return false;
    uart->Instance->TDR = (uint32_t)data;
    return true;
#else
    (void)data;
    return false;
#endif
}

uint32_t HAL_UART_FillTXFIFO(UART_HandleTypeDef *uart, const uint8_t *data, uint32_t length)
{
    uint32_t sent = 0U;
    if (!hal_uart_available(uart) || (data == 0)) return 0U;
    while ((sent < length) && HAL_UART_TransmitDataCheck(uart, data[sent])) {
        sent++;
    }
    return sent;
}

bool HAL_UART_IsBusy(UART_HandleTypeDef *uart)
{
    if (!hal_uart_available(uart)) return false;
#if (HAL_UART_STM32_HAS_UART == 1U)
    HAL_UART_StateTypeDef state = HAL_UART_GetState(uart);
    return (state == HAL_UART_STATE_BUSY) ||
           (state == HAL_UART_STATE_BUSY_TX) ||
           (state == HAL_UART_STATE_BUSY_RX) ||
           (state == HAL_UART_STATE_BUSY_TX_RX) ||
           (__HAL_UART_GET_FLAG(uart, UART_FLAG_BUSY) != RESET) ||
           (((uart->Instance->CR1 & USART_CR1_TE) != 0U) &&
            (__HAL_UART_GET_FLAG(uart, UART_FLAG_TC) == RESET));
#else
    return false;
#endif
}

bool Platform_UART_IsAvailable(const platform_uart_t *uart)
{
    return (uart != 0) && hal_uart_available(uart->regs);
}

uint32_t Platform_UART_GetPendingInterrupt(const platform_uart_t *uart)
{
    return (uart == 0) ? 0U : HAL_UART_GetPendingInterrupt(uart->regs);
}

bool Platform_UART_IsRXFIFOEmpty(const platform_uart_t *uart)
{
    return (uart == 0) || HAL_UART_IsRXFIFOEmpty(uart->regs);
}

uint8_t Platform_UART_ReceiveData(const platform_uart_t *uart)
{
    return (uart == 0) ? 0U : HAL_UART_ReceiveData(uart->regs);
}

bool Platform_UART_TransmitDataCheck(const platform_uart_t *uart, uint8_t data)
{
    return (uart != 0) && HAL_UART_TransmitDataCheck(uart->regs, data);
}

uint32_t Platform_UART_FillTXFIFO(const platform_uart_t *uart, const uint8_t *data, uint32_t length)
{
    return (uart == 0) ? 0U : HAL_UART_FillTXFIFO(uart->regs, data, length);
}

bool Platform_UART_IsBusy(const platform_uart_t *uart)
{
    return (uart != 0) && HAL_UART_IsBusy(uart->regs);
}
