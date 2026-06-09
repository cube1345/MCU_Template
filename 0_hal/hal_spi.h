#ifndef __HAL_SPI_H
#define __HAL_SPI_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"

#if defined(HAL_SPI_MODULE_ENABLED) && defined(__has_include)
#if __has_include("stm32f1xx_hal_spi.h")
#include "stm32f1xx_hal_spi.h"
#define HAL_SPI_STM32_HAS_SPI          (1U)
#endif
#endif

#ifndef HAL_SPI_STM32_HAS_SPI
#define HAL_SPI_STM32_HAS_SPI          (0U)
#endif

#if (HAL_SPI_STM32_HAS_SPI == 0U)
typedef void SPI_HandleTypeDef;
#endif

#ifndef HAL_SPI_TIMEOUT_MS
#define HAL_SPI_TIMEOUT_MS             (100U)
#endif

typedef enum
{
    HAL_SPI_PORT_0 = 0U,
    HAL_SPI_PORT_1 = 1U
} hal_spi_port_t;

typedef enum
{
    HAL_SPI_STATUS_OK = 0U,
    HAL_SPI_STATUS_ERROR_PARAM,
    HAL_SPI_STATUS_ERROR_TIMEOUT,
    HAL_SPI_STATUS_ERROR_BUSY,
    HAL_SPI_STATUS_ERROR_TRANSFER
} hal_spi_status_t;

typedef SPI_HandleTypeDef *platform_spi_regs_t;

typedef struct {
    platform_spi_regs_t regs;
} platform_spi_t;

void hal_spi_init(void);
uint32_t hal_spi_get_bus_speed_hz(hal_spi_port_t spi_port);
bool hal_spi_is_available(hal_spi_port_t spi_port);

hal_spi_status_t hal_spi_transmit(hal_spi_port_t spi_port,
                                  const uint8_t *tx_data,
                                  uint16_t length);
hal_spi_status_t hal_spi_receive(hal_spi_port_t spi_port,
                                 uint8_t *rx_data,
                                 uint16_t length);
hal_spi_status_t hal_spi_transmit_receive(hal_spi_port_t spi_port,
                                          const uint8_t *tx_data,
                                          uint8_t *rx_data,
                                          uint16_t length);
hal_spi_status_t hal_spi_write_then_read(hal_spi_port_t spi_port,
                                         const uint8_t *tx_data,
                                         uint16_t tx_length,
                                         uint8_t *rx_data,
                                         uint16_t rx_length);
hal_spi_status_t hal_spi_transfer_byte(hal_spi_port_t spi_port,
                                       uint8_t tx_data,
                                       uint8_t *rx_data);

hal_spi_status_t Platform_SPI_Transmit(const platform_spi_t *spi,
                                       const uint8_t *tx_data,
                                       uint16_t length);
hal_spi_status_t Platform_SPI_Receive(const platform_spi_t *spi,
                                      uint8_t *rx_data,
                                      uint16_t length);
hal_spi_status_t Platform_SPI_TransmitReceive(const platform_spi_t *spi,
                                              const uint8_t *tx_data,
                                              uint8_t *rx_data,
                                              uint16_t length);
hal_spi_status_t Platform_SPI_WriteThenRead(const platform_spi_t *spi,
                                            const uint8_t *tx_data,
                                            uint16_t tx_length,
                                            uint8_t *rx_data,
                                            uint16_t rx_length);
bool Platform_SPI_IsBusy(const platform_spi_t *spi);

#endif
