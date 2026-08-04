/**
 * @file    hal_spi.c
 * @brief   Board-independent SPI transfer API implementation.
 * @details Provides blocking 8-bit SPI transfers over TI DriverLib. Board code
 *          owns pinmux, clock, chip-select, and optional SysConfig bindings.
 * @author  cube
 * @date    2026-08-05
 * @note    Updated: 2026-08-05
 */

#include "hal_spi.h"

static hal_spi_status_t hal_spi_validate_transfer(SPI_Regs *spi_instance,
    const uint8_t *tx_data, const uint8_t *rx_data, uint16_t length)
{
    if ((spi_instance == (SPI_Regs *)0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    if ((tx_data == (const uint8_t *)0) && (rx_data == (const uint8_t *)0)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    return HAL_SPI_STATUS_OK;
}

hal_spi_status_t hal_spi_init(SPI_Regs *spi_instance)
{
    if (spi_instance == (SPI_Regs *)0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    if (DL_SPI_isEnabled(spi_instance) == false) {
        DL_SPI_enable(spi_instance);
    }

    return HAL_SPI_STATUS_OK;
}

hal_spi_status_t hal_spi_enable(SPI_Regs *spi_instance)
{
    if (spi_instance == (SPI_Regs *)0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    DL_SPI_enable(spi_instance);
    return HAL_SPI_STATUS_OK;
}

hal_spi_status_t hal_spi_disable(SPI_Regs *spi_instance)
{
    if (spi_instance == (SPI_Regs *)0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    DL_SPI_disable(spi_instance);
    return HAL_SPI_STATUS_OK;
}

bool hal_spi_is_busy(SPI_Regs *spi_instance)
{
    if (spi_instance == (SPI_Regs *)0) {
        return false;
    }

    return DL_SPI_isBusy(spi_instance);
}

hal_spi_status_t hal_spi_write8(SPI_Regs *spi_instance,
    const uint8_t *data, uint16_t length)
{
    if (data == (const uint8_t *)0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    return hal_spi_transfer8(spi_instance, data, (uint8_t *)0, length,
        HAL_SPI_DUMMY_DATA_DEFAULT);
}

hal_spi_status_t hal_spi_read8(SPI_Regs *spi_instance, uint8_t *data,
    uint16_t length, uint8_t dummy_data)
{
    if (data == (uint8_t *)0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    return hal_spi_transfer8(spi_instance, (const uint8_t *)0, data, length,
        dummy_data);
}

hal_spi_status_t hal_spi_transfer8(SPI_Regs *spi_instance,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t length,
    uint8_t dummy_data)
{
    hal_spi_status_t status;
    uint16_t index;

    status = hal_spi_validate_transfer(spi_instance, tx_data, rx_data, length);
    if (status != HAL_SPI_STATUS_OK) {
        return status;
    }

    for (index = 0U; index < length; index++) {
        uint8_t tx_byte = dummy_data;
        uint8_t rx_byte;

        if (tx_data != (const uint8_t *)0) {
            tx_byte = tx_data[index];
        }

        DL_SPI_transmitDataBlocking8(spi_instance, tx_byte);
        rx_byte = DL_SPI_receiveDataBlocking8(spi_instance);

        if (rx_data != (uint8_t *)0) {
            rx_data[index] = rx_byte;
        }
    }

    while (DL_SPI_isBusy(spi_instance) == true) {
    }

    return HAL_SPI_STATUS_OK;
}
