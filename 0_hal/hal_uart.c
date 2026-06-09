#include "hal_uart.h"

uint32_t HAL_UART_GetPendingInterrupt(UART_HandleTypeDef *uart)
{
    uint32_t pending = 0U;

    if (uart == 0) {
        return 0U;
    }

#if (HAL_UART_STM32_HAS_UART == 1U)
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) != RESET) {
        pending |= UART_FLAG_RXNE;
    }
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_TXE) != RESET) {
        pending |= UART_FLAG_TXE;
    }
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_TC) != RESET) {
        pending |= UART_FLAG_TC;
    }
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_ORE) != RESET) {
        pending |= UART_FLAG_ORE;
    }
#else
    (void)uart;
#endif

    return pending;
}

bool HAL_UART_IsRXFIFOEmpty(UART_HandleTypeDef *uart)
{
    if (uart == 0) {
        return true;
    }
#if (HAL_UART_STM32_HAS_UART == 1U)
    return (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) == RESET);
#else
    (void)uart;
    return true;
#endif
}

uint8_t HAL_UART_ReceiveData(UART_HandleTypeDef *uart)
{
    if (uart == 0) {
        return 0U;
    }
#if (HAL_UART_STM32_HAS_UART == 1U)
    return (uint8_t)(uart->Instance->DR & 0xFFU);
#else
    (void)uart;
    return 0U;
#endif
}

bool HAL_UART_TransmitDataCheck(UART_HandleTypeDef *uart, uint8_t data)
{
    if (uart == 0) {
        return false;
    }
#if (HAL_UART_STM32_HAS_UART == 1U)
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_TXE) == RESET) {
        return false;
    }

    uart->Instance->DR = (uint32_t)data;
    return true;
#else
    (void)uart;
    (void)data;
    return false;
#endif
}

uint32_t HAL_UART_FillTXFIFO(UART_HandleTypeDef *uart, const uint8_t *data, uint32_t length)
{
    uint32_t sent = 0U;

    if ((uart == 0) || (data == 0)) {
        return 0U;
    }

#if (HAL_UART_STM32_HAS_UART == 1U)
    while ((sent < length) && HAL_UART_TransmitDataCheck(uart, data[sent])) {
        sent++;
    }
#else
    (void)uart;
    (void)data;
    (void)length;
#endif

    return sent;
}

bool HAL_UART_IsBusy(UART_HandleTypeDef *uart)
{
    if (uart == 0) {
        return false;
    }

#if (HAL_UART_STM32_HAS_UART == 1U)
    HAL_UART_StateTypeDef state;

    state = HAL_UART_GetState(uart);
    return (state == HAL_UART_STATE_BUSY) ||
           (state == HAL_UART_STATE_BUSY_TX) ||
           (state == HAL_UART_STATE_BUSY_RX) ||
           (state == HAL_UART_STATE_BUSY_TX_RX);
#else
    (void)uart;
    return false;
#endif
}

uint32_t Platform_UART_GetPendingInterrupt(const platform_uart_t *uart)
{
    if (uart == 0) {
        return 0;
    }
    return HAL_UART_GetPendingInterrupt(uart->regs);
}

bool Platform_UART_IsRXFIFOEmpty(const platform_uart_t *uart)
{
    if (uart == 0) {
        return true;
    }
    return HAL_UART_IsRXFIFOEmpty(uart->regs);
}

uint8_t Platform_UART_ReceiveData(const platform_uart_t *uart)
{
    if (uart == 0) {
        return 0U;
    }
    return HAL_UART_ReceiveData(uart->regs);
}

bool Platform_UART_TransmitDataCheck(const platform_uart_t *uart, uint8_t data)
{
    if (uart == 0) {
        return false;
    }
    return HAL_UART_TransmitDataCheck(uart->regs, data);
}

uint32_t Platform_UART_FillTXFIFO(const platform_uart_t *uart, const uint8_t *data, uint32_t length)
{
    if (uart == 0) {
        return 0U;
    }
    return HAL_UART_FillTXFIFO(uart->regs, data, length);
}

bool Platform_UART_IsBusy(const platform_uart_t *uart)
{
    if (uart == 0) {
        return false;
    }
    return HAL_UART_IsBusy(uart->regs);
}
