#ifndef MYUSED_MYBOARDS_UART_BOARD_H
#define MYUSED_MYBOARDS_UART_BOARD_H

#include "../0_hal/hal_uart.h"

/** @brief 当前板级 UART 实例及其中断号绑定。 */
typedef struct
{
    UART_Regs *regs;
    IRQn_Type int_irqn;
} uart_board_t;

extern uart_board_t g_uart0_camera;
extern uart_board_t g_uart2_debug;

void UART_Board_Init(void);

#endif
