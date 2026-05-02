/**
 * @file    hal_gpio.h
 * @brief   HAL 层 GPIO 通用接口
 * @author  cube
 * @date    2026-04
 *
 * 提供两套 GPIO 操作接口：
 * - HAL_GPIO_* 过程式接口，直接传入 GPIO_Regs* + pin mask
 * - Platform_GPIO_* 对象式接口，传入 platform_gpio_t 指针，内置 NULL 检查
 *
 * 所有实现底层调用 TI DriverLib 的 DL_GPIO_* 函数。
 * Board 层和 Device 层应通过本接口操作 GPIO，禁止直接调用 DL_GPIO_*。
 */

#ifndef  __HAL_GPIO_H
#define  __HAL_GPIO_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/** @brief GPIO 端口类型（GPIO_Regs 指针别名） */
typedef GPIO_Regs *platform_gpio_port_t;
/** @brief GPIO 引脚类型（pin mask） */
typedef uint32_t platform_gpio_pin_t;

/** @brief GPIO 对象，绑定端口和引脚 */
typedef struct {
    platform_gpio_port_t port;
    platform_gpio_pin_t pin;
} platform_gpio_t;

/* ---- 过程式接口：直接传入端口和引脚 ---- */

uint8_t HAL_GPIO_ReadPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin, uint8_t Value);
void HAL_GPIO_TogglePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void HAL_GPIO_SetPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void HAL_GPIO_ClearPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);

/* ---- 对象式接口：传入 platform_gpio_t 指针，内置 NULL 检查 ---- */

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value);
void Platform_GPIO_Toggle(const platform_gpio_t *gpio);
uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio);
void Platform_GPIO_Set(const platform_gpio_t *gpio);
void Platform_GPIO_Clear(const platform_gpio_t *gpio);

#endif
