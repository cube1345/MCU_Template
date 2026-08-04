/**
 * @file    device_spi.h
 * @brief   Device-layer independent SPI transport contract.
 * @details Defines a SPI endpoint and transfer operations without exposing TI
 *          DriverLib, SysConfig, GPIO, or a specific SPI instance. Board code
 *          adapts hardware SPI and chip-select handling to this contract.
 * @author  cube
 * @date    2026-08-05
 * @note    Updated: 2026-08-05
 */

#ifndef DEVICE_SPI_H
#define DEVICE_SPI_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Transport-layer result shared by SPI bus adapters and Device code. */
typedef enum
{
    DEVICE_SPI_STATUS_OK = 0U,
    DEVICE_SPI_STATUS_ERROR_PARAM,
    DEVICE_SPI_STATUS_ERROR_UNSUPPORTED,
    DEVICE_SPI_STATUS_ERROR_TRANSFER
} device_spi_status_t;

/** @brief Board-provided SPI transport operations. */
typedef struct
{
    device_spi_status_t (*init)(void *context);
    device_spi_status_t (*select)(void *context);
    device_spi_status_t (*deselect)(void *context);
    device_spi_status_t (*transfer)(void *context, const uint8_t *tx_data,
        uint8_t *rx_data, uint16_t length);
} device_spi_ops_t;

/** @brief A SPI endpoint bound to Board transport operations and context. */
typedef struct
{
    const device_spi_ops_t *ops;
    void *context;
} device_spi_t;

static inline bool DeviceSpi_IsValid(const device_spi_t *spi)
{
    return (spi != (const device_spi_t *)0) &&
           (spi->ops != (const device_spi_ops_t *)0);
}

static inline device_spi_status_t DeviceSpi_Init(const device_spi_t *spi)
{
    if (DeviceSpi_IsValid(spi) == false) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    if (spi->ops->init == (device_spi_status_t (*)(void *))0) {
        return DEVICE_SPI_STATUS_OK;
    }

    return spi->ops->init(spi->context);
}

static inline device_spi_status_t DeviceSpi_Select(const device_spi_t *spi)
{
    if (DeviceSpi_IsValid(spi) == false) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    if (spi->ops->select == (device_spi_status_t (*)(void *))0) {
        return DEVICE_SPI_STATUS_OK;
    }

    return spi->ops->select(spi->context);
}

static inline device_spi_status_t DeviceSpi_Deselect(const device_spi_t *spi)
{
    if (DeviceSpi_IsValid(spi) == false) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    if (spi->ops->deselect == (device_spi_status_t (*)(void *))0) {
        return DEVICE_SPI_STATUS_OK;
    }

    return spi->ops->deselect(spi->context);
}

static inline device_spi_status_t DeviceSpi_Transfer(const device_spi_t *spi,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)
{
    device_spi_status_t status;

    if ((DeviceSpi_IsValid(spi) == false) || (length == 0U) ||
        ((tx_data == (const uint8_t *)0) && (rx_data == (uint8_t *)0))) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    if (spi->ops->transfer == (device_spi_status_t (*)(void *,
        const uint8_t *, uint8_t *, uint16_t))0) {
        return DEVICE_SPI_STATUS_ERROR_UNSUPPORTED;
    }

    status = DeviceSpi_Select(spi);
    if (status != DEVICE_SPI_STATUS_OK) {
        return status;
    }

    status = spi->ops->transfer(spi->context, tx_data, rx_data, length);
    (void)DeviceSpi_Deselect(spi);
    return status;
}

static inline device_spi_status_t DeviceSpi_Write(const device_spi_t *spi,
    const uint8_t *data, uint16_t length)
{
    if (data == (const uint8_t *)0) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    return DeviceSpi_Transfer(spi, data, (uint8_t *)0, length);
}

static inline device_spi_status_t DeviceSpi_Read(const device_spi_t *spi,
    uint8_t *data, uint16_t length)
{
    if (data == (uint8_t *)0) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    return DeviceSpi_Transfer(spi, (const uint8_t *)0, data, length);
}

#endif /* DEVICE_SPI_H */
