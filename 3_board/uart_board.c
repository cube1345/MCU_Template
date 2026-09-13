/**
 * @file uart_board.c
 * @brief Board-owned UART object with no default hardware binding.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#include "uart_board.h"

static platform_uart_t g_uart;

void UART_Board_Init(void)
{
    /* Template default: no board UART binding. */
}

void UART_Board_Bind(const platform_uart_t *uart)
{
    if (uart == 0) {
        g_uart = (platform_uart_t){0};
    } else {
        g_uart = *uart;
    }
}

const platform_uart_t *UART_Board_GetDevice(void)
{
    return Platform_UART_IsAvailable(&g_uart) ? &g_uart : 0;
}
