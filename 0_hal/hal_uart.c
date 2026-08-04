#include "hal_uart.h"

uint32_t HAL_UART_GetPendingInterrupt(UART_Regs *uart)
{
    if (uart == (UART_Regs *)0) {
        return 0U;
    }

    return DL_UART_Main_getPendingInterrupt(uart);
}

bool HAL_UART_IsRXFIFOEmpty(UART_Regs *uart)
{
    if (uart == (UART_Regs *)0) {
        return true;
    }

    return DL_UART_Main_isRXFIFOEmpty(uart);
}

uint8_t HAL_UART_ReceiveData(UART_Regs *uart)
{
    if (uart == (UART_Regs *)0) {
        return 0U;
    }

    return DL_UART_Main_receiveData(uart);
}

bool HAL_UART_TransmitDataCheck(UART_Regs *uart, uint8_t data)
{
    if (uart == (UART_Regs *)0) {
        return false;
    }

    return DL_UART_Main_transmitDataCheck(uart, data);
}

uint32_t HAL_UART_FillTXFIFO(UART_Regs *uart, const uint8_t *data, uint32_t length)
{
    if ((uart == (UART_Regs *)0) || (data == (const uint8_t *)0) ||
        (length == 0U)) {
        return 0U;
    }

    /* DriverLib 此参数缺少 const 限定，但其实现只读取 TX 缓冲区。 */
    return DL_UART_Main_fillTXFIFO(uart, (uint8_t *)data, length);
}

bool HAL_UART_IsBusy(UART_Regs *uart)
{
    if (uart == (UART_Regs *)0) {
        return false;
    }

    return DL_UART_Main_isBusy(uart);
}
