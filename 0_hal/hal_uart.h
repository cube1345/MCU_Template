#ifndef __HAL_UART_H
#define __HAL_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <ti/driverlib/dl_uart_main.h>

/* 通用 HAL 接口：直接传入 UART 寄存器基址。 */
uint32_t HAL_UART_GetPendingInterrupt(UART_Regs *uart);
bool HAL_UART_IsRXFIFOEmpty(UART_Regs *uart);
uint8_t HAL_UART_ReceiveData(UART_Regs *uart);
bool HAL_UART_TransmitDataCheck(UART_Regs *uart, uint8_t data);
uint32_t HAL_UART_FillTXFIFO(UART_Regs *uart, const uint8_t *data, uint32_t length);
bool HAL_UART_IsBusy(UART_Regs *uart);

#endif
