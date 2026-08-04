/**
 * @file    spi_board.c
 * @brief   Current board SPI instance and SysConfig macro binding.
 * @details This file may be compiled with no SysConfig SPI instance present.
 *          Missing SPI instances are never referenced outside defined guards.
 * @author  cube
 * @date    2026-08-05
 * @note    Updated: 2026-08-05
 */

#include "spi_board.h"

#include "hal_spi.h"
#include "ti_msp_dl_config.h"

static bool g_spi_board_initialized;

static SPI_Regs *spi_board_get_regs(void *context)
{
    spi_board_context_t *spi_context = (spi_board_context_t *)context;

    if ((spi_context != (spi_board_context_t *)0) &&
        (spi_context->regs != (SPI_Regs *)0)) {
        return spi_context->regs;
    }

    return SPI_Board_GetDefault();
}

static uint8_t spi_board_get_dummy_data(void *context)
{
    spi_board_context_t *spi_context = (spi_board_context_t *)context;

    if (spi_context != (spi_board_context_t *)0) {
        return spi_context->dummy_data;
    }

    return HAL_SPI_DUMMY_DATA_DEFAULT;
}

static device_spi_status_t spi_board_convert_status(hal_spi_status_t status)
{
    if (status == HAL_SPI_STATUS_OK) {
        return DEVICE_SPI_STATUS_OK;
    }

    if (status == HAL_SPI_STATUS_ERROR_PARAM) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    return DEVICE_SPI_STATUS_ERROR_TRANSFER;
}

static device_spi_status_t spi_board_init(void *context)
{
    SPI_Board_Init();

    if (spi_board_get_regs(context) == (SPI_Regs *)0) {
        return DEVICE_SPI_STATUS_ERROR_PARAM;
    }

    return DEVICE_SPI_STATUS_OK;
}

static void spi_board_set_selected(void *context, bool selected)
{
    spi_board_context_t *spi_context = (spi_board_context_t *)context;

    if ((spi_context != (spi_board_context_t *)0) &&
        (spi_context->select_fn != (spi_board_select_fn_t)0)) {
        spi_context->select_fn(spi_context->select_context, selected);
    }
}

static device_spi_status_t spi_board_select(void *context)
{
    spi_board_set_selected(context, true);
    return DEVICE_SPI_STATUS_OK;
}

static device_spi_status_t spi_board_deselect(void *context)
{
    spi_board_set_selected(context, false);
    return DEVICE_SPI_STATUS_OK;
}

static device_spi_status_t spi_board_transfer(void *context,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)
{
    return spi_board_convert_status(
        hal_spi_transfer8(spi_board_get_regs(context), tx_data, rx_data, length,
            spi_board_get_dummy_data(context)));
}

const device_spi_ops_t g_spi_board_default_ops = {
    .init = spi_board_init,
    .select = spi_board_select,
    .deselect = spi_board_deselect,
    .transfer = spi_board_transfer,
};

void SPI_Board_Init(void)
{
    bool initialized = false;

    if (g_spi_board_initialized == true) {
        return;
    }

#if defined(SPI_0_INST)
    if (hal_spi_init(SPI_0_INST) == HAL_SPI_STATUS_OK) {
        initialized = true;
    }
#endif

#if defined(SPI_1_INST)
    if (hal_spi_init(SPI_1_INST) == HAL_SPI_STATUS_OK) {
        initialized = true;
    }
#endif

    g_spi_board_initialized = initialized;
}

SPI_Regs *SPI_Board_GetDefault(void)
{
#if defined(SPI_0_INST)
    return SPI_0_INST;
#elif defined(SPI_1_INST)
    return SPI_1_INST;
#else
    return (SPI_Regs *)0;
#endif
}

device_spi_t SPI_Board_MakeDevice(spi_board_context_t *context)
{
    device_spi_t spi = {
        .ops = &g_spi_board_default_ops,
        .context = context,
    };

    return spi;
}
