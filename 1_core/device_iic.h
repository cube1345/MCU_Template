/**
 * @file    device_iic.h
 * @brief   Device-layer independent IIC transport contract.
 * @details Defines a 7-bit-addressed IIC endpoint and its operations without
 *          exposing TI DriverLib, SysConfig, GPIO, or a specific IIC instance.
 *          Board code adapts hardware or software IIC to this contract.
 * @author  cube
 * @date    2026-08-03
 * @note    Updated: 2026-08-03
 */

#ifndef DEVICE_IIC_H
#define DEVICE_IIC_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Transport-layer result shared by IIC bus adapters and Device code. */
typedef enum
{
    DEVICE_IIC_STATUS_OK = 0U,
    DEVICE_IIC_STATUS_ERROR_PARAM,
    DEVICE_IIC_STATUS_ERROR_UNSUPPORTED,
    DEVICE_IIC_STATUS_ERROR_TRANSFER
} device_iic_status_t;

/**
 * @brief Board-provided IIC transport operations.
 * @note  Register operations use one-byte register addresses and are optional.
 */
typedef struct
{
    device_iic_status_t (*init)(void *context);
    device_iic_status_t (*probe)(void *context, uint8_t address);
    device_iic_status_t (*write)(void *context, uint8_t address,
        const uint8_t *data, uint16_t length);
    device_iic_status_t (*read)(void *context, uint8_t address,
        uint8_t *data, uint16_t length);
    device_iic_status_t (*write_reg8)(void *context, uint8_t address,
        uint8_t reg_addr, uint8_t reg_data);
    device_iic_status_t (*read_reg8)(void *context, uint8_t address,
        uint8_t reg_addr, uint8_t *reg_data);
    device_iic_status_t (*write_reg8s)(void *context, uint8_t address,
        uint8_t reg_addr, const uint8_t *data, uint16_t length);
    device_iic_status_t (*read_reg8s)(void *context, uint8_t address,
        uint8_t reg_addr, uint8_t *data, uint16_t length);
} device_iic_ops_t;

/**
 * @brief An IIC endpoint bound to a Board transport, context, and 7-bit address.
 */
typedef struct
{
    const device_iic_ops_t *ops;
    void *context;
    uint8_t address;
} device_iic_t;

static inline bool DeviceIic_IsValid(const device_iic_t *iic)
{
    return (iic != (const device_iic_t *)0) &&
           (iic->ops != (const device_iic_ops_t *)0) &&
           (iic->address <= 0x7FU);
}

static inline device_iic_status_t DeviceIic_Init(const device_iic_t *iic)
{
    if (DeviceIic_IsValid(iic) == false) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->init == (device_iic_status_t (*)(void *))0) {
        return DEVICE_IIC_STATUS_OK;
    }

    return iic->ops->init(iic->context);
}

static inline device_iic_status_t DeviceIic_Probe(const device_iic_t *iic)
{
    if (DeviceIic_IsValid(iic) == false) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->probe == (device_iic_status_t (*)(void *, uint8_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->probe(iic->context, iic->address);
}

static inline device_iic_status_t DeviceIic_Write(const device_iic_t *iic,
    const uint8_t *data, uint16_t length)
{
    if ((DeviceIic_IsValid(iic) == false) ||
        (data == (const uint8_t *)0) || (length == 0U)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->write == (device_iic_status_t (*)(void *, uint8_t,
        const uint8_t *, uint16_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->write(iic->context, iic->address, data, length);
}

static inline device_iic_status_t DeviceIic_Read(const device_iic_t *iic,
    uint8_t *data, uint16_t length)
{
    if ((DeviceIic_IsValid(iic) == false) ||
        (data == (uint8_t *)0) || (length == 0U)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->read == (device_iic_status_t (*)(void *, uint8_t,
        uint8_t *, uint16_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->read(iic->context, iic->address, data, length);
}

static inline device_iic_status_t DeviceIic_WriteReg8(const device_iic_t *iic,
    uint8_t reg_addr, uint8_t reg_data)
{
    if (DeviceIic_IsValid(iic) == false) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->write_reg8 == (device_iic_status_t (*)(void *, uint8_t,
        uint8_t, uint8_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->write_reg8(iic->context, iic->address, reg_addr, reg_data);
}

static inline device_iic_status_t DeviceIic_ReadReg8(const device_iic_t *iic,
    uint8_t reg_addr, uint8_t *reg_data)
{
    if ((DeviceIic_IsValid(iic) == false) || (reg_data == (uint8_t *)0)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->read_reg8 == (device_iic_status_t (*)(void *, uint8_t,
        uint8_t, uint8_t *))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->read_reg8(iic->context, iic->address, reg_addr, reg_data);
}

static inline device_iic_status_t DeviceIic_WriteReg8s(const device_iic_t *iic,
    uint8_t reg_addr, const uint8_t *data, uint16_t length)
{
    if ((DeviceIic_IsValid(iic) == false) ||
        (data == (const uint8_t *)0) || (length == 0U)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->write_reg8s == (device_iic_status_t (*)(void *, uint8_t,
        uint8_t, const uint8_t *, uint16_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->write_reg8s(iic->context, iic->address, reg_addr, data, length);
}

static inline device_iic_status_t DeviceIic_ReadReg8s(const device_iic_t *iic,
    uint8_t reg_addr, uint8_t *data, uint16_t length)
{
    if ((DeviceIic_IsValid(iic) == false) ||
        (data == (uint8_t *)0) || (length == 0U)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    if (iic->ops->read_reg8s == (device_iic_status_t (*)(void *, uint8_t,
        uint8_t, uint8_t *, uint16_t))0) {
        return DEVICE_IIC_STATUS_ERROR_UNSUPPORTED;
    }

    return iic->ops->read_reg8s(iic->context, iic->address, reg_addr, data, length);
}

#endif /* DEVICE_IIC_H */
