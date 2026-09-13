/**
 * @file uart_board.h
 * @brief Optional Board UART object binding API.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#ifndef MYUSED_MYBOARDS_UART_BOARD_H
#define MYUSED_MYBOARDS_UART_BOARD_H

#include "hal_uart.h"

/** @brief Preserve the optional binding; CubeMX initializes its handle first. */
void UART_Board_Init(void);

/** @brief Copy an initialized Board UART binding; NULL clears it.
 * @note Bind during startup, before any concurrent UART users run.
 */
void UART_Board_Bind(const platform_uart_t *uart);

/** @brief Return the Board-owned UART object, or NULL when unavailable. */
const platform_uart_t *UART_Board_GetDevice(void);

#endif
