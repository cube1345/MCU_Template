/**
 * @file hal_iic.h
 * @brief Instance-based blocking STM32 I2C adapter.
 * @details Board supplies an initialized CubeMX handle and timeout. Calls use
 *          unshifted 7-bit addresses; callers serialize access to each handle.
 * @author cube
 * @date 2026-09-13
 * @note Updated: 2026-09-13
 */
#ifndef __HAL_IIC_H
#define __HAL_IIC_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32u5xx_hal.h"

#if defined(HAL_I2C_MODULE_ENABLED)
#include "stm32u5xx_hal_i2c.h"
#define HAL_IIC_STM32_HAS_I2C          (1U)
#endif

#ifndef HAL_IIC_STM32_HAS_I2C
#define HAL_IIC_STM32_HAS_I2C          (0U)
#endif

#if (HAL_IIC_STM32_HAS_I2C == 0U)
typedef void I2C_HandleTypeDef;
#endif

/* BSP IIC 传输模式定义值：轮询模式。 */
#define HAL_IIC_TRANSFER_MODE_POLL      (0U)

/* BSP IIC 传输模式定义值：中断模式。 */
#define HAL_IIC_TRANSFER_MODE_IRQ       (1U)

/*
 * BSP IIC 当前传输模式选择宏。
 * STM32 当前实现使用阻塞式 HAL I2C API；该宏保留给上层兼容。
 */
#ifndef HAL_IIC_TRANSFER_MODE
#define HAL_IIC_TRANSFER_MODE           HAL_IIC_TRANSFER_MODE_IRQ
#endif

/* 寄存器读链路强制轮询开关，STM32 当前实现中保留给上层兼容。 */
#ifndef HAL_IIC_FORCE_POLL_REG_READ
#define HAL_IIC_FORCE_POLL_REG_READ     (0U)
#endif

/**
  * @brief  BSP IIC 接口返回状态定义，用于上层统一判断传输结果。
  */
typedef enum
{
    HAL_IIC_STATUS_OK = 0U,
    HAL_IIC_STATUS_ERROR_PARAM,
    HAL_IIC_STATUS_ERROR_TIMEOUT,
    HAL_IIC_STATUS_ERROR_BUSY,
    HAL_IIC_STATUS_ERROR_NACK,
    HAL_IIC_STATUS_ERROR_ARB_LOST,
    HAL_IIC_STATUS_ERROR_TRANSFER
} hal_iic_status_t;

/** @brief Last operation diagnostics; unsupported timing/IRQ fields stay zero. */
typedef struct
{
    uint16_t transfer_length;
    uint8_t last_stage;
    uint8_t finish_stage;
    uint32_t last_irq_iidx;
    uint32_t last_controller_status;
    uint32_t finish_controller_status;
    uint32_t last_raw_interrupt_status;
    uint32_t finish_raw_interrupt_status;
    uint32_t rxfifo_trigger_count;
    uint32_t rxdone_count;
    uint32_t wait_notify_value;
    uint32_t wait_rtos_ticks;
    uint32_t finish_rtos_tick;
    uint32_t wake_lag_rtos_ticks;
    uint32_t result_code;
    uint8_t completed_by_timeout;
    uint32_t wait_bus_us;
    uint32_t wait_idle_us;
    uint32_t clear_us;
    uint32_t prepare_us;
    uint32_t wait_us;
    uint32_t poll_tx_us;
    uint32_t poll_rx_us;
    uint32_t poll_tail_us;
    uint32_t total_us;
} hal_iic_profile_t;

/** @brief Board-owned bus binding; CubeMX initializes the handle first. */
typedef struct {
    I2C_HandleTypeDef *handle; /**< Borrowed hardware handle. */
    uint32_t timeout_ms;      /**< HAL blocking timeout in milliseconds. */
} hal_iic_cfg_t;

/** @brief One bus configuration and its own last-operation diagnostics. */
typedef struct {
    hal_iic_cfg_t cfg;
    hal_iic_profile_t profile;
} hal_iic_t;
/** @brief Clear diagnostics and enable an initialized handle; null is a no-op. */
void hal_iic_init(hal_iic_t *bus);

/** @brief Return configured TIMINGR value, or zero for unavailable hardware. */
uint32_t hal_iic_get_bus_timing(const hal_iic_t *bus);

/** @brief Borrow this bus's profile, or return NULL for a null bus. */
const hal_iic_profile_t *hal_iic_get_profile(const hal_iic_t *bus);

/**
 * @brief Probe an unshifted 7-bit address with two HAL readiness trials.
 * @return HAL_IIC_STATUS_OK on ACK, otherwise a parameter or transport error.
 * @note All transfers block and require a running HAL tick; do not call in ISR.
 */
hal_iic_status_t hal_iic_check_device(hal_iic_t *bus, uint8_t device_addr);

/** @brief Receive nonzero bytes into a nonnull buffer, without a register address. */
hal_iic_status_t hal_iic_read_bytes(hal_iic_t *bus, uint8_t device_addr, uint8_t *read_data, uint16_t read_length);

/** @brief Transmit nonzero bytes from a nonnull buffer, without a register address. */
hal_iic_status_t hal_iic_write_bytes(hal_iic_t *bus, uint8_t device_addr, const uint8_t *write_data, uint16_t write_length);

/** @brief Read one byte at an 8-bit register address into a nonnull pointer. */
hal_iic_status_t hal_iic_read_reg8(hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data);

/** @brief Write one byte at an 8-bit register address. */
hal_iic_status_t hal_iic_write_reg8(hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data);

/** @brief Read nonzero bytes using an 8-bit register address and repeated START. */
hal_iic_status_t hal_iic_read_reg8s(hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);

/** @brief Write nonzero bytes after an 8-bit register address. */
hal_iic_status_t hal_iic_write_reg8s(hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);

/**
 * @brief HAL tick based wait for injected device startup delays.
 * @param[in] context Unused callback context; NULL is allowed.
 * @param[in] ms Delay in milliseconds; requires a running HAL tick, outside ISR.
 */
void hal_iic_delay_ms(void *context, uint32_t ms);

#endif
