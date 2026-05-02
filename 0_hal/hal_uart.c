#include "hal_uart.h"

uint32_t HAL_UART_GetPendingInterrupt(UART_Regs *uart)
{
    return DL_UART_Main_getPendingInterrupt(uart);
}

bool HAL_UART_IsRXFIFOEmpty(UART_Regs *uart)
{
    return DL_UART_Main_isRXFIFOEmpty(uart);
}

uint8_t HAL_UART_ReceiveData(UART_Regs *uart)
{
    return DL_UART_Main_receiveData(uart);
}

bool HAL_UART_TransmitDataCheck(UART_Regs *uart, uint8_t data)
{
    return DL_UART_Main_transmitDataCheck(uart, data);
}

uint32_t HAL_UART_FillTXFIFO(UART_Regs *uart, const uint8_t *data, uint32_t length)
{
    return DL_UART_Main_fillTXFIFO(uart, data, length);
}

bool HAL_UART_IsBusy(UART_Regs *uart)
{
    return DL_UART_Main_isBusy(uart);
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
