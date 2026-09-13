/**
 * @file hal_uart.h
 * @brief Public byte-oriented STM32U5 UART object interface.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#ifndef __HAL_UART_H
#define __HAL_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx_hal.h"

#if defined(HAL_UART_MODULE_ENABLED)
#include "stm32u5xx_hal_uart.h"
#define HAL_UART_STM32_HAS_UART        (1U)
#endif

#ifndef HAL_UART_STM32_HAS_UART
#define HAL_UART_STM32_HAS_UART        (0U)
#endif

#if (HAL_UART_STM32_HAS_UART == 0U)
typedef void UART_HandleTypeDef;
#endif

typedef UART_HandleTypeDef *platform_uart_regs_t;
typedef IRQn_Type platform_uart_irqn_t;

/** @brief Byte-oriented UART object; Board supplies its configured handle/IRQ.
 * @note Direct FIFO calls must not share a handle with active HAL IT/DMA transfers.
 */
typedef struct {
    platform_uart_regs_t regs;
    platform_uart_irqn_t int_irqn;
} platform_uart_t;

/* 过程式接口：直接传入 STM32 UART 句柄 */
/** @brief Raw-handle counterpart of Platform_UART_GetPendingInterrupt. */
uint32_t HAL_UART_GetPendingInterrupt(UART_HandleTypeDef *uart);
/** @brief Return true when the raw handle is unavailable or RX FIFO empty. */
bool HAL_UART_IsRXFIFOEmpty(UART_HandleTypeDef *uart);
/** @brief Pop one byte from an available raw handle, or return zero. */
uint8_t HAL_UART_ReceiveData(UART_HandleTypeDef *uart);
/** @brief Attempt one byte write without waiting. */
bool HAL_UART_TransmitDataCheck(UART_HandleTypeDef *uart, uint8_t data);
/** @brief Return the number of bytes accepted without waiting. */
uint32_t HAL_UART_FillTXFIFO(UART_HandleTypeDef *uart, const uint8_t *data, uint32_t length);
/** @brief Query HAL/hardware activity; unavailable handles return false. */
bool HAL_UART_IsBusy(UART_HandleTypeDef *uart);

/* 对象式接口：传入 platform_uart_t 指针，带 NULL 检查 */
/** @brief Whether an initialized peripheral binding is available in this build. */
bool Platform_UART_IsAvailable(const platform_uart_t *uart);
/** @brief Return enabled RXNE/TXE/TC/ORE flags without acknowledging them. */
uint32_t Platform_UART_GetPendingInterrupt(const platform_uart_t *uart);
/** @brief True if unavailable or the receive FIFO is empty. */
bool Platform_UART_IsRXFIFOEmpty(const platform_uart_t *uart);
/** @brief Pop one byte, or return zero when empty/unavailable. */
uint8_t Platform_UART_ReceiveData(const platform_uart_t *uart);
/** @brief Enqueue one byte without waiting; false means no byte was written. */
bool Platform_UART_TransmitDataCheck(const platform_uart_t *uart, uint8_t data);
/** @brief Enqueue up to length bytes without waiting; return bytes accepted. */
uint32_t Platform_UART_FillTXFIFO(const platform_uart_t *uart, const uint8_t *data, uint32_t length);
/** @brief True while HAL transfers or hardware transmission/reception are active. */
bool Platform_UART_IsBusy(const platform_uart_t *uart);

#endif
