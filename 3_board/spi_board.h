/**
 * @file    spi_board.h
 * @brief   Current board SPI instance and chip-select binding.
 * @details Keeps SysConfig-generated SPI instance macros and optional
 *          chip-select callbacks out of HAL and Device code.
 * @author  cube
 * @date    2026-08-05
 * @note    Updated: 2026-08-05
 */

#ifndef __SPI_BOARD_H
#define __SPI_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <ti/driverlib/dl_spi.h>

#include "device_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_board_select_fn_t)(void *context, bool selected);

/**
 * @brief Board-side SPI endpoint context.
 * @note  select_fn may be NULL when SysConfig hardware CS or no CS is used.
 */
typedef struct
{
    SPI_Regs *regs;
    spi_board_select_fn_t select_fn;
    void *select_context;
    uint8_t dummy_data;
} spi_board_context_t;

/** @brief Initialize currently configured SPI controllers. */
void SPI_Board_Init(void);

/**
 * @brief Get the default configured SPI controller.
 * @return First configured SPI instance; NULL when SysConfig has no SPI.
 */
SPI_Regs *SPI_Board_GetDefault(void);

/**
 * @brief Build a Device SPI endpoint backed by Board SPI operations.
 * @param[in] context Board context; may contain a specific SPI instance and CS.
 * @return Device SPI endpoint using g_spi_board_default_ops.
 */
device_spi_t SPI_Board_MakeDevice(spi_board_context_t *context);

/**
 * @brief Core SPI transport operations backed by the Board default SPI bus.
 * @note  A device descriptor may set context to a spi_board_context_t.
 */
extern const device_spi_ops_t g_spi_board_default_ops;

#ifdef __cplusplus
}
#endif

#endif /* __SPI_BOARD_H */
