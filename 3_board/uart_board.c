#include "uart_board.h"
#include "ti_msp_dl_config.h"

uart_board_t g_uart0_camera = {
    .regs = UART_0_INST,
    .int_irqn = UART_0_INST_INT_IRQN,
};

uart_board_t g_uart2_debug = {
    .regs = UART_2_INST,
    .int_irqn = UART_2_INST_INT_IRQN,
};

void UART_Board_Init(void)
{
    NVIC_EnableIRQ(g_uart0_camera.int_irqn);
    NVIC_EnableIRQ(g_uart2_debug.int_irqn);
}
