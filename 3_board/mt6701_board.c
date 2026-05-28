/**
 * @file    mt6701_board.c
 * @brief   MT6701 板级绑定层
 * @author  cube
 * @date    2026-05-02
 *
 * 职责:
 * - 定义两片 MT6701 在当前板上的具体引脚接线
 * - 通过 HAL_GPIO_InitOutput/InitInput 初始化 GPIO IOMUX
 * - 填充 mt6701_bus_cfg_t 并注入 Device 层 MT6701_Init()
 *
 * 本文件不直接调用任何 DL_GPIO_* 函数, 所有 GPIO 操作通过 HAL 层完成。
 *
 * 硬件接线:
 *   共享 SCLK → PA3, 共享 DO → PA2
 *   M1 CSN → PA4, M2 CSN → PB0
 */

#include "mt6701_board.h"
#include "hal_gpio.h"
#include "../2_device/mt6701_driver.h"
#include "ti_msp_dl_config.h"

/* ---- 板级引脚定义 -------------------------------------------------- */

/* IOMUX */
#define MT6701_SCLK_IOMUX        IOMUX_PINCM8    /* PA3 */
#define MT6701_DO_IOMUX          IOMUX_PINCM7    /* PA2 */
#define MT6701_M1_CSN_IOMUX      IOMUX_PINCM9    /* PA4 */
#define MT6701_M2_CSN_IOMUX      IOMUX_PINCM12   /* PB0 */

/* 端口 + 引脚号 */
#define MT6701_SCLK_PORT         GPIOA
#define MT6701_SCLK_PIN          DL_GPIO_PIN_3

#define MT6701_DO_PORT           GPIOA
#define MT6701_DO_PIN            DL_GPIO_PIN_2

#define MT6701_M1_CSN_PORT       GPIOA
#define MT6701_M1_CSN_PIN        DL_GPIO_PIN_4

#define MT6701_M2_CSN_PORT       GPIOB
#define MT6701_M2_CSN_PIN        DL_GPIO_PIN_0

/* ------------------------------------------------------------------ */
/* 公共接口                                                           */
/* ------------------------------------------------------------------ */

void MT6701_Board_Init(void)
{
    /*
     * 1. 通过 HAL 层初始化所有 GPIO IOMUX
     *    Board 层不直接调用 DL_GPIO_initDigital* / DL_GPIO_setPins / DL_GPIO_clearPins
     */
    HAL_GPIO_InitOutput(MT6701_M1_CSN_IOMUX);
    HAL_GPIO_WritePin(MT6701_M1_CSN_PORT, MT6701_M1_CSN_PIN, 1U);

    HAL_GPIO_InitOutput(MT6701_M2_CSN_IOMUX);
    HAL_GPIO_WritePin(MT6701_M2_CSN_PORT, MT6701_M2_CSN_PIN, 1U);

    HAL_GPIO_InitOutput(MT6701_SCLK_IOMUX);
    HAL_GPIO_WritePin(MT6701_SCLK_PORT, MT6701_SCLK_PIN, 0U);

    HAL_GPIO_InitInput(MT6701_DO_IOMUX);

    /*
     * 2. 填充引脚配置并注入 Device 层
     *    Device 层不包含任何具体引脚号, 所有引脚信息由此注入
     */
    const mt6701_bus_cfg_t bus_cfg = {
        .sclk = { MT6701_SCLK_PORT, MT6701_SCLK_PIN },
        .dio  = { MT6701_DO_PORT,   MT6701_DO_PIN   },
        .csn  = {
            [MT6701_AXIS_M1] = { MT6701_M1_CSN_PORT, MT6701_M1_CSN_PIN },
            [MT6701_AXIS_M2] = { MT6701_M2_CSN_PORT, MT6701_M2_CSN_PIN },
        },
    };

    MT6701_Init(&bus_cfg);
}
