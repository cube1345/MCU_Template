#include "hal_spi.h"

#if (HAL_SPI_STM32_HAS_SPI == 1U) && defined(__has_include)
#if __has_include("spi.h")
#include "spi.h"
#define HAL_SPI_STM32_HAS_PORT0        (1U)
#endif
#endif

#ifndef HAL_SPI_STM32_HAS_PORT0
#define HAL_SPI_STM32_HAS_PORT0        (0U)
#endif

static SPI_HandleTypeDef *hal_spi_get_handle(hal_spi_port_t spi_port)
{
#if (HAL_SPI_STM32_HAS_PORT0 == 1U)
    switch (spi_port) {
    case HAL_SPI_PORT_0:
        return &hspi2;

    case HAL_SPI_PORT_1:
    default:
        return 0;
    }
#else
    (void)spi_port;
    return 0;
#endif
}

static hal_spi_status_t hal_spi_convert_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return HAL_SPI_STATUS_OK;
    }
    if (status == HAL_TIMEOUT) {
        return HAL_SPI_STATUS_ERROR_TIMEOUT;
    }
    if (status == HAL_BUSY) {
        return HAL_SPI_STATUS_ERROR_BUSY;
    }

    return HAL_SPI_STATUS_ERROR_TRANSFER;
}

#if (HAL_SPI_STM32_HAS_SPI == 1U)
static uint32_t hal_spi_prescaler_to_div(uint32_t prescaler)
{
    switch (prescaler) {
    case SPI_BAUDRATEPRESCALER_2:
        return 2U;
    case SPI_BAUDRATEPRESCALER_4:
        return 4U;
    case SPI_BAUDRATEPRESCALER_8:
        return 8U;
    case SPI_BAUDRATEPRESCALER_16:
        return 16U;
    case SPI_BAUDRATEPRESCALER_32:
        return 32U;
    case SPI_BAUDRATEPRESCALER_64:
        return 64U;
    case SPI_BAUDRATEPRESCALER_128:
        return 128U;
    case SPI_BAUDRATEPRESCALER_256:
        return 256U;
    default:
        return 0U;
    }
}

static uint32_t hal_spi_get_input_clock_hz(SPI_HandleTypeDef *handle)
{
    if ((handle == 0) || (handle->Instance == 0)) {
        return 0U;
    }

    if (handle->Instance == SPI1) {
        return HAL_RCC_GetPCLK2Freq();
    }

    return HAL_RCC_GetPCLK1Freq();
}
#endif

void hal_spi_init(void)
{
    SPI_HandleTypeDef *handle = hal_spi_get_handle(HAL_SPI_PORT_0);

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    if (handle != 0) {
        __HAL_SPI_ENABLE(handle);
    }
#else
    (void)handle;
#endif
}

uint32_t hal_spi_get_bus_speed_hz(hal_spi_port_t spi_port)
{
    SPI_HandleTypeDef *handle = hal_spi_get_handle(spi_port);

    if (handle == 0) {
        return 0U;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    {
        uint32_t divider = hal_spi_prescaler_to_div(handle->Init.BaudRatePrescaler);
        uint32_t input_clock = hal_spi_get_input_clock_hz(handle);

        if (divider == 0U) {
            return 0U;
        }

        return input_clock / divider;
    }
#else
    (void)handle;
    return 0U;
#endif
}

bool hal_spi_is_available(hal_spi_port_t spi_port)
{
    return (hal_spi_get_handle(spi_port) != 0);
}

hal_spi_status_t hal_spi_transmit(hal_spi_port_t spi_port,
                                  const uint8_t *tx_data,
                                  uint16_t length)
{
    SPI_HandleTypeDef *handle = hal_spi_get_handle(spi_port);
    HAL_StatusTypeDef hal_status;

    if ((handle == 0) || (tx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_Transmit(handle, (uint8_t *)tx_data, length, HAL_SPI_TIMEOUT_MS);
#else
    (void)tx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t hal_spi_receive(hal_spi_port_t spi_port,
                                 uint8_t *rx_data,
                                 uint16_t length)
{
    SPI_HandleTypeDef *handle = hal_spi_get_handle(spi_port);
    HAL_StatusTypeDef hal_status;

    if ((handle == 0) || (rx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_Receive(handle, rx_data, length, HAL_SPI_TIMEOUT_MS);
#else
    (void)rx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t hal_spi_transmit_receive(hal_spi_port_t spi_port,
                                          const uint8_t *tx_data,
                                          uint8_t *rx_data,
                                          uint16_t length)
{
    SPI_HandleTypeDef *handle = hal_spi_get_handle(spi_port);
    HAL_StatusTypeDef hal_status;

    if ((handle == 0) || (tx_data == 0) || (rx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_TransmitReceive(handle, (uint8_t *)tx_data, rx_data,
                                         length, HAL_SPI_TIMEOUT_MS);
#else
    (void)tx_data;
    (void)rx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t hal_spi_write_then_read(hal_spi_port_t spi_port,
                                         const uint8_t *tx_data,
                                         uint16_t tx_length,
                                         uint8_t *rx_data,
                                         uint16_t rx_length)
{
    hal_spi_status_t status;

    if ((tx_data == 0) || (tx_length == 0U) || (rx_data == 0) || (rx_length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    status = hal_spi_transmit(spi_port, tx_data, tx_length);
    if (status != HAL_SPI_STATUS_OK) {
        return status;
    }

    return hal_spi_receive(spi_port, rx_data, rx_length);
}

hal_spi_status_t hal_spi_transfer_byte(hal_spi_port_t spi_port,
                                       uint8_t tx_data,
                                       uint8_t *rx_data)
{
    if (rx_data == 0) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    return hal_spi_transmit_receive(spi_port, &tx_data, rx_data, 1U);
}

hal_spi_status_t Platform_SPI_Transmit(const platform_spi_t *spi,
                                       const uint8_t *tx_data,
                                       uint16_t length)
{
    HAL_StatusTypeDef hal_status;

    if ((spi == 0) || (spi->regs == 0) || (tx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_Transmit(spi->regs, (uint8_t *)tx_data, length, HAL_SPI_TIMEOUT_MS);
#else
    (void)spi;
    (void)tx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t Platform_SPI_Receive(const platform_spi_t *spi,
                                      uint8_t *rx_data,
                                      uint16_t length)
{
    HAL_StatusTypeDef hal_status;

    if ((spi == 0) || (spi->regs == 0) || (rx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_Receive(spi->regs, rx_data, length, HAL_SPI_TIMEOUT_MS);
#else
    (void)spi;
    (void)rx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t Platform_SPI_TransmitReceive(const platform_spi_t *spi,
                                              const uint8_t *tx_data,
                                              uint8_t *rx_data,
                                              uint16_t length)
{
    HAL_StatusTypeDef hal_status;

    if ((spi == 0) || (spi->regs == 0) || (tx_data == 0) || (rx_data == 0) || (length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    hal_status = HAL_SPI_TransmitReceive(spi->regs, (uint8_t *)tx_data, rx_data,
                                         length, HAL_SPI_TIMEOUT_MS);
#else
    (void)spi;
    (void)tx_data;
    (void)rx_data;
    (void)length;
    hal_status = HAL_ERROR;
#endif

    return hal_spi_convert_status(hal_status);
}

hal_spi_status_t Platform_SPI_WriteThenRead(const platform_spi_t *spi,
                                            const uint8_t *tx_data,
                                            uint16_t tx_length,
                                            uint8_t *rx_data,
                                            uint16_t rx_length)
{
    hal_spi_status_t status;

    if ((tx_data == 0) || (tx_length == 0U) || (rx_data == 0) || (rx_length == 0U)) {
        return HAL_SPI_STATUS_ERROR_PARAM;
    }

    status = Platform_SPI_Transmit(spi, tx_data, tx_length);
    if (status != HAL_SPI_STATUS_OK) {
        return status;
    }

    return Platform_SPI_Receive(spi, rx_data, rx_length);
}

bool Platform_SPI_IsBusy(const platform_spi_t *spi)
{
    if ((spi == 0) || (spi->regs == 0)) {
        return false;
    }

#if (HAL_SPI_STM32_HAS_SPI == 1U)
    return (HAL_SPI_GetState(spi->regs) == HAL_SPI_STATE_BUSY);
#else
    (void)spi;
    return false;
#endif
}
