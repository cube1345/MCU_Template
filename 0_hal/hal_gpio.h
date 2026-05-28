/**
 * @file    hal_gpio.h
 * @brief   HAL 层 GPIO 通用接口
 * @author  cube
 * @date    2026-04
 *
 * 提供两套 GPIO 操作接口：
 * - STM32 HAL_GPIO_* 过程式接口，直接传入 GPIO_TypeDef* + pin mask
 * - Platform_GPIO_* 对象式接口，传入 platform_gpio_t 指针，内置 NULL 检查
 *
 * Board 层和 Device 层应通过本接口或 STM32 HAL GPIO 接口操作 GPIO，
 * 不再直接依赖 TI DriverLib。
 */

#ifndef  __HAL_GPIO_H
#define  __HAL_GPIO_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief GPIO 端口类型（GPIO_TypeDef 指针别名） */
typedef GPIO_TypeDef *platform_gpio_port_t;
/** @brief GPIO 引脚类型（pin mask） */
typedef uint16_t platform_gpio_pin_t;

/** @brief GPIO 对象，绑定端口和引脚 */
typedef struct {
    platform_gpio_port_t port;
    platform_gpio_pin_t pin;
} platform_gpio_t;

/* ---- GPIO 便捷接口 ---- */

void HAL_GPIO_SetPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_ClearPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- GPIO 初始化接口 ---- */

void HAL_GPIO_InitOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_InitInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_InitPeripheralOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                                   uint32_t alternate);
void HAL_GPIO_EnableOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- 对象式接口：传入 platform_gpio_t 指针，内置 NULL 检查 ---- */

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value);
void Platform_GPIO_Toggle(const platform_gpio_t *gpio);
uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio);
void Platform_GPIO_Set(const platform_gpio_t *gpio);
void Platform_GPIO_Clear(const platform_gpio_t *gpio);

#endif
