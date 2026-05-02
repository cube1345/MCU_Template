/**
 * @file    oled_board.c
 * @brief   OLED 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 提供 OLED 软件 I2C 的 SCL/SDA GPIO 写函数
 * - 通过 OLED_I2C_Init() 将 SCL/SDA 回调注入 Device 层
 * - 提供 OLED_Board_Init() 统一初始化入口
 *
 * OLED 使用软件 I2C（非硬件 I2C 外设），SCL/SDA 线控通过函数指针注入。
 * GPIO 操作通过 HAL 层 HAL_GPIO_WritePin()，不直接调用 DL_GPIO_*。
 */

#include "oled_board.h"
#include "hal_gpio.h"
#include "ti_msp_dl_config.h"

/** @brief 软件 I2C SCL 线写函数，通过 HAL 层操作 GPIO */
static void OLED_SCL_Write(uint8_t state)
{
    HAL_GPIO_WritePin(OLED_PORT, OLED_SCL0_PIN, state);
}

/** @brief 软件 I2C SDA 线写函数，通过 HAL 层操作 GPIO */
static void OLED_SDA_Write(uint8_t state)
{
    HAL_GPIO_WritePin(OLED_PORT, OLED_SDA0_PIN, state);
}

/** @brief 初始化 OLED I2C 通信层，注入 SCL/SDA 回调到 Device 层 */
void OLED_Driver_Init(void)
{
	OLED_I2C_Init(OLED_SCL_Write, OLED_SDA_Write);
}

/** @brief OLED 板级初始化：配置 I2C 回调并初始化 OLED 控制器 */
void OLED_Board_Init(void)
{
	OLED_Driver_Init();
	OLED_Init();
}
