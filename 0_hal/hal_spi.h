/**
 * @file    hal_spi.h
 * @brief   Board-independent SPI transfer API.
 * @details Wraps TI DriverLib SPI calls using caller-injected SPI_Regs
 *          instances. This layer does not reference SysConfig-generated
 *          instance, pin, chip-select, or IRQ macros.
 * @author  cube
 * @date    2026-08-05
 * @note    Updated: 2026-08-05
 */

#ifndef __HAL_SPI_H
#define __HAL_SPI_H

#include <stdbool.h>
#include <stdint.h>

#include <ti/driverlib/dl_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SPI_DUMMY_DATA_DEFAULT      (0xFFU)

/** @brief Generic SPI HAL return status. */
typedef enum
{
    HAL_SPI_STATUS_OK = 0U,
    HAL_SPI_STATUS_ERROR_PARAM,
    HAL_SPI_STATUS_ERROR_TRANSFER
} hal_spi_status_t;

/**
 * @brief Register a configured SPI controller instance for blocking transfers.
 * @param[in] spi_instance SPI controller register base supplied by Board code.
 * @return HAL_SPI_STATUS_OK on success; HAL_SPI_STATUS_ERROR_PARAM otherwise.
 */
hal_spi_status_t hal_spi_init(SPI_Regs *spi_instance);

/**
 * @brief Enable a SPI controller instance.
 * @param[in] spi_instance SPI controller register base.
 * @return HAL_SPI_STATUS_OK on success; HAL_SPI_STATUS_ERROR_PARAM otherwise.
 */
hal_spi_status_t hal_spi_enable(SPI_Regs *spi_instance);

/**
 * @brief Disable a SPI controller instance.
 * @param[in] spi_instance SPI controller register base.
 * @return HAL_SPI_STATUS_OK on success; HAL_SPI_STATUS_ERROR_PARAM otherwise.
 */
hal_spi_status_t hal_spi_disable(SPI_Regs *spi_instance);

/**
 * @brief Check whether a SPI controller is currently busy.
 * @param[in] spi_instance SPI controller register base.
 * @return true when the controller reports busy; false for idle or invalid input.
 */
bool hal_spi_is_busy(SPI_Regs *spi_instance);

/**
 * @brief Write 8-bit frames and discard simultaneously received bytes.
 * @param[in] spi_instance SPI controller register base.
 * @param[in] data Buffer to transmit.
 * @param[in] length Number of bytes to transmit.
 * @return HAL_SPI_STATUS_OK on success; error status otherwise.
 */
hal_spi_status_t hal_spi_write8(SPI_Regs *spi_instance,
    const uint8_t *data, uint16_t length);

/**
 * @brief Read 8-bit frames by transmitting a dummy byte for each frame.
 * @param[in] spi_instance SPI controller register base.
 * @param[out] data Buffer that receives bytes.
 * @param[in] length Number of bytes to receive.
 * @param[in] dummy_data Byte clocked out while reading.
 * @return HAL_SPI_STATUS_OK on success; error status otherwise.
 */
hal_spi_status_t hal_spi_read8(SPI_Regs *spi_instance, uint8_t *data,
    uint16_t length, uint8_t dummy_data);

/**
 * @brief Full-duplex 8-bit transfer.
 * @param[in] spi_instance SPI controller register base.
 * @param[in] tx_data Optional transmit buffer; dummy_data is used when NULL.
 * @param[out] rx_data Optional receive buffer; received bytes are discarded when NULL.
 * @param[in] length Number of bytes to transfer.
 * @param[in] dummy_data Byte clocked out when tx_data is NULL.
 * @return HAL_SPI_STATUS_OK on success; error status otherwise.
 */
hal_spi_status_t hal_spi_transfer8(SPI_Regs *spi_instance,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t length,
    uint8_t dummy_data);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_SPI_H */
