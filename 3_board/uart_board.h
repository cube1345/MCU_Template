#ifndef MYUSED_MYBOARDS_UART_BOARD_H
#define MYUSED_MYBOARDS_UART_BOARD_H

#include "../0_hal/hal_uart.h"

extern platform_uart_t g_uart0_camera;
extern platform_uart_t g_uart2_debug;

void UART_Board_Init(void);

#endif
