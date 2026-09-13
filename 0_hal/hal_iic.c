/**
 * @file hal_iic.c
 * @brief Blocking I2C operations on explicitly supplied bus objects.
 * @details Hardware setup belongs to CubeMX/Board; diagnostics belong to bus.
 * @author cube
 * @date 2026-09-13
 * @note Updated: 2026-09-13
 */
#include "hal_iic.h"

#include <string.h>

#if (HAL_IIC_STM32_HAS_I2C != 0U)
/** @brief Shift the validated 7-bit address into the vendor API format. */
static uint16_t hal_iic_hal_address(uint8_t device_addr)
{
    return (uint16_t)((uint16_t)device_addr << 1U);
}

/** @brief Preserve timeout/busy and decode HAL transport failure flags. */
static hal_iic_status_t hal_iic_convert_status(
    I2C_HandleTypeDef *handle, HAL_StatusTypeDef hal_status)
{
    uint32_t error;

    if (hal_status == HAL_OK) return HAL_IIC_STATUS_OK;
    if (hal_status == HAL_TIMEOUT) return HAL_IIC_STATUS_ERROR_TIMEOUT;
    if (hal_status == HAL_BUSY) return HAL_IIC_STATUS_ERROR_BUSY;

    error = HAL_I2C_GetError(handle);
    if ((error & HAL_I2C_ERROR_AF) != 0U) return HAL_IIC_STATUS_ERROR_NACK;
    if ((error & HAL_I2C_ERROR_ARLO) != 0U) return HAL_IIC_STATUS_ERROR_ARB_LOST;
    return HAL_IIC_STATUS_ERROR_TRANSFER;
}
#endif

static void hal_iic_profile_begin(hal_iic_t *bus, uint16_t length)
{
    if (bus == NULL) return;
    memset(&bus->profile, 0, sizeof(bus->profile));
    bus->profile.transfer_length = length;
}

static hal_iic_status_t hal_iic_finish(hal_iic_t *bus, hal_iic_status_t status)
{
    if (bus != NULL) {
        bus->profile.result_code = (uint32_t)status;
        bus->profile.completed_by_timeout =
            (status == HAL_IIC_STATUS_ERROR_TIMEOUT) ? 1U : 0U;
    }
    return status;
}

/** @brief Validate a bus/address without touching unavailable hardware. */
static hal_iic_status_t hal_iic_validate_bus(
    const hal_iic_t *bus, uint8_t device_addr)
{
    if ((bus == NULL) || (bus->cfg.handle == NULL) || (device_addr > 0x7FU)) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (bus->cfg.handle->Instance == NULL) return HAL_IIC_STATUS_ERROR_PARAM;
    return HAL_IIC_STATUS_OK;
#else
    return HAL_IIC_STATUS_ERROR_TRANSFER;
#endif
}

/** @brief Start diagnostics even when rejecting a malformed transfer. */
static hal_iic_status_t hal_iic_prepare(
    hal_iic_t *bus, uint8_t device_addr, const void *data, uint16_t length)
{
    hal_iic_status_t status;

    hal_iic_profile_begin(bus, length);
    if ((data == NULL) || (length == 0U)) {
        return hal_iic_finish(bus, HAL_IIC_STATUS_ERROR_PARAM);
    }
    status = hal_iic_validate_bus(bus, device_addr);
    return hal_iic_finish(bus, status);
}

void hal_iic_init(hal_iic_t *bus)
{
    hal_iic_status_t status;

    hal_iic_profile_begin(bus, 0U);
    status = hal_iic_validate_bus(bus, 0U);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        __HAL_I2C_ENABLE(bus->cfg.handle);
    }
#endif
    (void)hal_iic_finish(bus, status);
}

uint32_t hal_iic_get_bus_timing(const hal_iic_t *bus)
{
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (hal_iic_validate_bus(bus, 0U) == HAL_IIC_STATUS_OK) {
        return bus->cfg.handle->Init.Timing;
    }
#else
    (void)bus;
#endif
    return 0U;
}

const hal_iic_profile_t *hal_iic_get_profile(const hal_iic_t *bus)
{
    return (bus == NULL) ? NULL : &bus->profile;
}

hal_iic_status_t hal_iic_check_device(hal_iic_t *bus, uint8_t device_addr)
{
    hal_iic_status_t status;

    hal_iic_profile_begin(bus, 0U);
    status = hal_iic_validate_bus(bus, device_addr);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(
            bus->cfg.handle, hal_iic_hal_address(device_addr), 2U,
            bus->cfg.timeout_ms);
        status = hal_iic_convert_status(bus->cfg.handle, hal_status);
    }
#endif
    return hal_iic_finish(bus, status);
}

hal_iic_status_t hal_iic_write_bytes(
    hal_iic_t *bus, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    hal_iic_status_t status = hal_iic_prepare(bus, device_addr, write_data, write_length);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(
            bus->cfg.handle, hal_iic_hal_address(device_addr), (uint8_t *)write_data,
            write_length, bus->cfg.timeout_ms);
        status = hal_iic_convert_status(bus->cfg.handle, hal_status);
    }
#endif
    return hal_iic_finish(bus, status);
}

hal_iic_status_t hal_iic_read_bytes(
    hal_iic_t *bus, uint8_t device_addr, uint8_t *read_data, uint16_t read_length)
{
    hal_iic_status_t status = hal_iic_prepare(bus, device_addr, read_data, read_length);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(
            bus->cfg.handle, hal_iic_hal_address(device_addr), read_data,
            read_length, bus->cfg.timeout_ms);
        status = hal_iic_convert_status(bus->cfg.handle, hal_status);
    }
#endif
    return hal_iic_finish(bus, status);
}

hal_iic_status_t hal_iic_write_reg8(
    hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data)
{
    return hal_iic_write_reg8s(bus, device_addr, reg_addr, &reg_data, 1U);
}

hal_iic_status_t hal_iic_read_reg8(
    hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data)
{
    return hal_iic_read_reg8s(bus, device_addr, reg_addr, reg_data, 1U);
}

hal_iic_status_t hal_iic_write_reg8s(
    hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    hal_iic_status_t status = hal_iic_prepare(bus, device_addr, write_data, write_length);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Write(
            bus->cfg.handle, hal_iic_hal_address(device_addr), reg_addr,
            I2C_MEMADD_SIZE_8BIT, (uint8_t *)write_data, write_length,
            bus->cfg.timeout_ms);
        status = hal_iic_convert_status(bus->cfg.handle, hal_status);
    }
#else
    (void)reg_addr;
#endif
    return hal_iic_finish(bus, status);
}

hal_iic_status_t hal_iic_read_reg8s(
    hal_iic_t *bus, uint8_t device_addr, uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    hal_iic_status_t status = hal_iic_prepare(bus, device_addr, read_data, read_length);
#if (HAL_IIC_STM32_HAS_I2C != 0U)
    if (status == HAL_IIC_STATUS_OK) {
        HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Read(
            bus->cfg.handle, hal_iic_hal_address(device_addr), reg_addr,
            I2C_MEMADD_SIZE_8BIT, read_data, read_length, bus->cfg.timeout_ms);
        status = hal_iic_convert_status(bus->cfg.handle, hal_status);
    }
#else
    (void)reg_addr;
#endif
    return hal_iic_finish(bus, status);
}

void hal_iic_delay_ms(void *context, uint32_t ms)
{
    (void)context;
    HAL_Delay(ms);
}
