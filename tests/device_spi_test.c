/**
 * @file device_spi_test.c
 * @brief Host test for the board-injected Device SPI endpoint contract.
 * @author cube
 * @date 2026-08-05
 * @note Updated: 2026-08-05
 */

#include <assert.h>
#include <stdint.h>

#include "device_spi.h"

typedef struct
{
    uint32_t init_count;
    uint32_t select_count;
    uint32_t deselect_count;
    uint32_t transfer_count;
    uint16_t last_length;
} device_spi_test_context_t;

static device_spi_status_t DeviceSpiTest_Init(void *context)
{
    device_spi_test_context_t *test_context = (device_spi_test_context_t *)context;

    test_context->init_count++;
    return DEVICE_SPI_STATUS_OK;
}

static device_spi_status_t DeviceSpiTest_Select(void *context)
{
    device_spi_test_context_t *test_context = (device_spi_test_context_t *)context;

    test_context->select_count++;
    return DEVICE_SPI_STATUS_OK;
}

static device_spi_status_t DeviceSpiTest_Deselect(void *context)
{
    device_spi_test_context_t *test_context = (device_spi_test_context_t *)context;

    test_context->deselect_count++;
    return DEVICE_SPI_STATUS_OK;
}

static device_spi_status_t DeviceSpiTest_Transfer(void *context,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)
{
    device_spi_test_context_t *test_context = (device_spi_test_context_t *)context;

    assert((tx_data != (const uint8_t *)0) || (rx_data != (uint8_t *)0));
    test_context->transfer_count++;
    test_context->last_length = length;
    if (rx_data != (uint8_t *)0) {
        rx_data[0] = 0xA5U;
    }
    return DEVICE_SPI_STATUS_OK;
}

int main(void)
{
    const device_spi_ops_t ops = {
        .init = DeviceSpiTest_Init,
        .select = DeviceSpiTest_Select,
        .deselect = DeviceSpiTest_Deselect,
        .transfer = DeviceSpiTest_Transfer,
    };
    const device_spi_ops_t no_transfer_ops = {0};
    device_spi_test_context_t test_context = {0};
    device_spi_t spi = {
        .ops = &ops,
        .context = &test_context,
    };
    device_spi_t no_transfer_spi = {
        .ops = &no_transfer_ops,
        .context = &test_context,
    };
    uint8_t tx_data[2] = {0x12U, 0x34U};
    uint8_t rx_data[2] = {0U, 0U};

    assert(DeviceSpi_IsValid(&spi) == true);
    assert(DeviceSpi_Init(&spi) == DEVICE_SPI_STATUS_OK);
    assert(test_context.init_count == 1U);

    assert(DeviceSpi_Write(&spi, tx_data, (uint16_t)sizeof(tx_data)) ==
        DEVICE_SPI_STATUS_OK);
    assert(test_context.transfer_count == 1U);
    assert(test_context.select_count == 1U);
    assert(test_context.deselect_count == 1U);

    assert(DeviceSpi_Read(&spi, rx_data, 1U) == DEVICE_SPI_STATUS_OK);
    assert(rx_data[0] == 0xA5U);
    assert(test_context.transfer_count == 2U);
    assert(test_context.select_count == 2U);
    assert(test_context.deselect_count == 2U);

    assert(DeviceSpi_Transfer(&spi, tx_data, rx_data, 1U) ==
        DEVICE_SPI_STATUS_OK);
    assert(rx_data[0] == 0xA5U);
    assert(test_context.transfer_count == 3U);
    assert(test_context.select_count == 3U);
    assert(test_context.deselect_count == 3U);

    assert(DeviceSpi_Read(&no_transfer_spi, rx_data, 1U) ==
        DEVICE_SPI_STATUS_ERROR_UNSUPPORTED);
    assert(DeviceSpi_Write(&spi, (const uint8_t *)0, 1U) ==
        DEVICE_SPI_STATUS_ERROR_PARAM);
    assert(DeviceSpi_Transfer(&spi, (const uint8_t *)0, (uint8_t *)0, 1U) ==
        DEVICE_SPI_STATUS_ERROR_PARAM);

    return 0;
}
