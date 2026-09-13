#include "hal_iic.h"

#include <string.h>

#if (HAL_IIC_STM32_HAS_I2C == 1U) && defined(__has_include)
#if __has_include("i2c.h")
#include "i2c.h"
#define HAL_IIC_STM32_HAS_PORT0        (1U)
#endif
#endif

#ifndef HAL_IIC_STM32_HAS_PORT0
#define HAL_IIC_STM32_HAS_PORT0        (0U)
#endif

#ifndef HAL_IIC_TIMEOUT_MS
#define HAL_IIC_TIMEOUT_MS              (100U)
#endif

static hal_iic_profile_t g_hal_iic0_profile;

static I2C_HandleTypeDef *hal_iic_get_handle(hal_iic_port_t iic_port)
{
#if (HAL_IIC_STM32_HAS_PORT0 == 1U)
    switch (iic_port) {
    case HAL_IIC_PORT_0:
        return &hi2c1;

    case HAL_IIC_PORT_1:
    default:
        return 0;
    }
#else
    (void)iic_port;
    return 0;
#endif
}

static hal_iic_profile_t *hal_iic_get_profile_slot(hal_iic_port_t iic_port)
{
    switch (iic_port) {
    case HAL_IIC_PORT_0:
        return &g_hal_iic0_profile;

    case HAL_IIC_PORT_1:
    default:
        return 0;
    }
}

static uint16_t hal_iic_get_hal_addr(uint8_t device_addr)
{
    return (uint16_t)((uint16_t)device_addr << 1U);
}

#if (HAL_IIC_STM32_HAS_I2C == 1U)
static hal_iic_status_t hal_iic_convert_status(I2C_HandleTypeDef *handle,
                                               HAL_StatusTypeDef status)
{
    uint32_t error;

    if (status == HAL_OK) {
        return HAL_IIC_STATUS_OK;
    }
    if (status == HAL_TIMEOUT) {
        return HAL_IIC_STATUS_ERROR_TIMEOUT;
    }
    if (status == HAL_BUSY) {
        return HAL_IIC_STATUS_ERROR_BUSY;
    }

    if (handle == 0) {
        return HAL_IIC_STATUS_ERROR_TRANSFER;
    }

    error = HAL_I2C_GetError(handle);
    if ((error & HAL_I2C_ERROR_AF) != 0U) {
        return HAL_IIC_STATUS_ERROR_NACK;
    }
    if ((error & HAL_I2C_ERROR_ARLO) != 0U) {
        return HAL_IIC_STATUS_ERROR_ARB_LOST;
    }

    return HAL_IIC_STATUS_ERROR_TRANSFER;
}
#else
static hal_iic_status_t hal_iic_convert_status(void *handle, HAL_StatusTypeDef status)
{
    (void)handle;
    (void)status;
    return HAL_IIC_STATUS_ERROR_TRANSFER;
}
#endif

static void hal_iic_profile_begin(hal_iic_port_t iic_port, uint16_t transfer_length)
{
    hal_iic_profile_t *profile = hal_iic_get_profile_slot(iic_port);

    if (profile == 0) {
        return;
    }

    memset(profile, 0, sizeof(*profile));
    profile->transfer_length = transfer_length;
}

static void hal_iic_profile_end(hal_iic_port_t iic_port, hal_iic_status_t status)
{
    hal_iic_profile_t *profile = hal_iic_get_profile_slot(iic_port);

    if (profile == 0) {
        return;
    }

    profile->result_code = (uint32_t)status;
}

void hal_iic_init(void)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(HAL_IIC_PORT_0);

#if (HAL_IIC_STM32_HAS_I2C == 1U)
    if (handle != 0) {
        __HAL_I2C_ENABLE(handle);
    }
#else
    (void)handle;
#endif
}

uint32_t hal_iic_get_bus_speed_hz(hal_iic_port_t iic_port)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);

    if (handle == 0) {
        return 0U;
    }

#if (HAL_IIC_STM32_HAS_I2C == 1U)
    return handle->Init.ClockSpeed;
#else
    (void)handle;
    return 0U;
#endif
}

const hal_iic_profile_t *hal_iic_get_profile(hal_iic_port_t iic_port)
{
    return hal_iic_get_profile_slot(iic_port);
}

hal_iic_status_t hal_iic_check_device(hal_iic_port_t iic_port, uint8_t device_addr)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);
    HAL_StatusTypeDef hal_status;
    hal_iic_status_t status;

    if (handle == 0) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }

    hal_iic_profile_begin(iic_port, 0U);
#if (HAL_IIC_STM32_HAS_I2C == 1U)
    hal_status = HAL_I2C_IsDeviceReady(handle, hal_iic_get_hal_addr(device_addr),
                                       2U, HAL_IIC_TIMEOUT_MS);
#else
    (void)device_addr;
    hal_status = HAL_ERROR;
#endif
    status = hal_iic_convert_status(handle, hal_status);
    hal_iic_profile_end(iic_port, status);

    return status;
}

hal_iic_status_t hal_iic_write_bytes(hal_iic_port_t iic_port, uint8_t device_addr,
                                     const uint8_t *write_data, uint16_t write_length)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);
    HAL_StatusTypeDef hal_status;
    hal_iic_status_t status;

    if ((handle == 0) || (write_data == 0) || (write_length == 0U)) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }

    hal_iic_profile_begin(iic_port, write_length);
#if (HAL_IIC_STM32_HAS_I2C == 1U)
    hal_status = HAL_I2C_Master_Transmit(handle, hal_iic_get_hal_addr(device_addr),
                                         (uint8_t *)write_data, write_length,
                                         HAL_IIC_TIMEOUT_MS);
#else
    (void)device_addr;
    hal_status = HAL_ERROR;
#endif
    status = hal_iic_convert_status(handle, hal_status);
    hal_iic_profile_end(iic_port, status);

    return status;
}

hal_iic_status_t hal_iic_read_bytes(hal_iic_port_t iic_port, uint8_t device_addr,
                                    uint8_t *read_data, uint16_t read_length)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);
    HAL_StatusTypeDef hal_status;
    hal_iic_status_t status;

    if ((handle == 0) || (read_data == 0) || (read_length == 0U)) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }

    hal_iic_profile_begin(iic_port, read_length);
#if (HAL_IIC_STM32_HAS_I2C == 1U)
    hal_status = HAL_I2C_Master_Receive(handle, hal_iic_get_hal_addr(device_addr),
                                        read_data, read_length, HAL_IIC_TIMEOUT_MS);
#else
    (void)device_addr;
    hal_status = HAL_ERROR;
#endif
    status = hal_iic_convert_status(handle, hal_status);
    hal_iic_profile_end(iic_port, status);

    return status;
}

hal_iic_status_t hal_iic_write_reg8(hal_iic_port_t iic_port, uint8_t device_addr,
                                    uint8_t reg_addr, uint8_t reg_data)
{
    return hal_iic_write_reg8s(iic_port, device_addr, reg_addr, &reg_data, 1U);
}

hal_iic_status_t hal_iic_read_reg8(hal_iic_port_t iic_port, uint8_t device_addr,
                                   uint8_t reg_addr, uint8_t *reg_data)
{
    return hal_iic_read_reg8s(iic_port, device_addr, reg_addr, reg_data, 1U);
}

hal_iic_status_t hal_iic_write_reg8s(hal_iic_port_t iic_port, uint8_t device_addr,
                                     uint8_t reg_addr, const uint8_t *write_data,
                                     uint16_t write_length)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);
    HAL_StatusTypeDef hal_status;
    hal_iic_status_t status;

    if ((handle == 0) || (write_data == 0) || (write_length == 0U)) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }

    hal_iic_profile_begin(iic_port, write_length);
#if (HAL_IIC_STM32_HAS_I2C == 1U)
    hal_status = HAL_I2C_Mem_Write(handle, hal_iic_get_hal_addr(device_addr),
                                   reg_addr, I2C_MEMADD_SIZE_8BIT,
                                   (uint8_t *)write_data, write_length,
                                   HAL_IIC_TIMEOUT_MS);
#else
    (void)device_addr;
    (void)reg_addr;
    hal_status = HAL_ERROR;
#endif
    status = hal_iic_convert_status(handle, hal_status);
    hal_iic_profile_end(iic_port, status);

    return status;
}

hal_iic_status_t hal_iic_read_reg8s(hal_iic_port_t iic_port, uint8_t device_addr,
                                    uint8_t reg_addr, uint8_t *read_data,
                                    uint16_t read_length)
{
    I2C_HandleTypeDef *handle = hal_iic_get_handle(iic_port);
    HAL_StatusTypeDef hal_status;
    hal_iic_status_t status;

    if ((handle == 0) || (read_data == 0) || (read_length == 0U)) {
        return HAL_IIC_STATUS_ERROR_PARAM;
    }

    hal_iic_profile_begin(iic_port, read_length);
#if (HAL_IIC_STM32_HAS_I2C == 1U)
    hal_status = HAL_I2C_Mem_Read(handle, hal_iic_get_hal_addr(device_addr),
                                  reg_addr, I2C_MEMADD_SIZE_8BIT,
                                  read_data, read_length, HAL_IIC_TIMEOUT_MS);
#else
    (void)device_addr;
    (void)reg_addr;
    hal_status = HAL_ERROR;
#endif
    status = hal_iic_convert_status(handle, hal_status);
    hal_iic_profile_end(iic_port, status);

    return status;
}
