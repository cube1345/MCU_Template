/**
 * @file    hal_gpio.h
 * @brief   HAL 层 GPIO 通用接口
 * @author  cube
 * @date    2026-04
 * @note Updated: 2026-09-14
 * @details Board configures clocks and safe initial levels before mode changes.
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
#include "stm32u5xx_hal.h"

#if defined(HAL_GPIO_MODULE_ENABLED)
#include "stm32u5xx_hal_gpio.h"
#define HAL_GPIO_STM32_HAS_GPIO        (1U)
#endif

#ifndef HAL_GPIO_STM32_HAS_GPIO
#define HAL_GPIO_STM32_HAS_GPIO        (0U)
#endif

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

/** @brief Set/clear pin masks; null/disabled bindings are safe no-ops. */
void HAL_GPIO_SetPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
/** @brief Drive a pin mask low without changing its mode. */
void HAL_GPIO_ClearPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- GPIO 初始化接口 ---- */

/** @brief Configure push-pull output; caller enables clock and initial level. */
void HAL_GPIO_InitOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
/** @brief Configure input without internal pull-up/down. */
void HAL_GPIO_InitInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
/** @brief Configure push-pull alternate function 0..15 without enabling clocks. */
void HAL_GPIO_InitPeripheralOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                                   uint32_t alternate);
/** @brief Compatibility no-op: STM32 mode initialization already enables output. */
void HAL_GPIO_EnableOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- 对象式接口：传入 platform_gpio_t 指针，内置 NULL 检查 ---- */

/** @brief Write a logical raw level; unavailable objects are ignored. */
void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value);
/** @brief Toggle the bound output level; unavailable objects are ignored. */
void Platform_GPIO_Toggle(const platform_gpio_t *gpio);
/** @brief Read the bound pin mask; return zero when unavailable. */
uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio);
/** @brief Drive a bound GPIO high. */
void Platform_GPIO_Set(const platform_gpio_t *gpio);
/** @brief Drive a bound GPIO low. */
void Platform_GPIO_Clear(const platform_gpio_t *gpio);

/** @brief Return one when a GPIO object is bound and the module is enabled. */
uint8_t Platform_GPIO_IsAvailable(const platform_gpio_t *gpio);
/** @brief Configure a bound object as output; Board enables its clock first. */
void Platform_GPIO_InitOutput(const platform_gpio_t *gpio);
/** @brief Configure a bound object as an input without pull-up/down. */
void Platform_GPIO_InitInput(const platform_gpio_t *gpio);
/** @brief Select an alternate function (0..15) on a bound object. */
void Platform_GPIO_InitPeripheralOutput(const platform_gpio_t *gpio, uint32_t alternate);

#endif
