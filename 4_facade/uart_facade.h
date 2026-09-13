/**
 * @file uart_facade.h
 * @brief Nonblocking application UART facade.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#ifndef MYUSED_MYFACADES_UART_FACADE_H
#define MYUSED_MYFACADES_UART_FACADE_H

#include "uart_board.h"

/** @brief Obtain the optional default Board UART; NULL means unavailable. */
static inline const platform_uart_t *UART_GetDevice(void)
{
    return UART_Board_GetDevice();
}

/** @brief Query whether the default Board UART is bound and enabled. */
static inline bool UART_IsAvailable(void)
{
    return Platform_UART_IsAvailable(UART_GetDevice());
}

/** @brief Nonblocking write; return the number of bytes accepted by TX FIFO. */
static inline uint32_t UART_Write(const uint8_t *data, uint32_t length)
{
    return Platform_UART_FillTXFIFO(UART_GetDevice(), data, length);
}

/** @brief Nonblocking read; return the number of bytes popped from RX FIFO. */
static inline uint32_t UART_Read(uint8_t *data, uint32_t length)
{
    const platform_uart_t *uart = UART_GetDevice();
    uint32_t received = 0U;

    if (data == 0) return 0U;
    while ((received < length) && !Platform_UART_IsRXFIFOEmpty(uart)) {
        data[received++] = Platform_UART_ReceiveData(uart);
    }
    return received;
}

#endif /* MYUSED_MYFACADES_UART_FACADE_H */
