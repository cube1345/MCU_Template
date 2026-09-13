/**
 * @file    hal_gpio_int.h
 * @brief   HAL 层 GPIO 中断状态接口
 * @author  cube
 * @date    2026-04
 * @note Updated: 2026-09-14
 * @details Board owns EXTI routing; status combines both edges and IMR1.
 *
 * 封装 STM32 HAL 的 GPIO EXTI 中断状态读写。
 * 从 hal_gpio.h 中独立出来，专门服务于 Board 层 ISR 中的中断状态查询和清除。
 */

#ifndef __HAL_GPIO_INT_H
#define __HAL_GPIO_INT_H

#include <stdint.h>
#include "hal_gpio.h"

#if defined(HAL_EXTI_MODULE_ENABLED) && defined(HAL_GPIO_MODULE_ENABLED)
#define HAL_GPIO_INT_STM32_HAS_EXTI    (1U)
#endif

#ifndef HAL_GPIO_INT_STM32_HAS_EXTI
#define HAL_GPIO_INT_STM32_HAS_EXTI    (0U)
#endif

/**
 * @brief  读取已使能的 GPIO 中断状态
 * @param  GPIOx GPIO 端口，STM32 EXTI pending 只与 pin 相关，当前参数保留用于接口统一
 * @param  GPIO_Pin 引脚掩码
 * @retval 中断状态位掩码
 */
uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  清除 GPIO 中断状态
 * @param  GPIOx GPIO 端口，STM32 EXTI pending 只与 pin 相关，当前参数保留用于接口统一
 * @param  GPIO_Pin 引脚掩码
 */
void HAL_GPIO_ClearInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/** @brief Return enabled rising/falling EXTI pending bits for a bound GPIO. */
uint32_t Platform_GPIO_GetEnabledInterruptStatus(const platform_gpio_t *gpio);
/** @brief Clear both edge pending bits for a bound GPIO. */
void Platform_GPIO_ClearInterruptStatus(const platform_gpio_t *gpio);

#endif
