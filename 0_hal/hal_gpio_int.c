/**
 * @file hal_gpio_int.c
 * @brief STM32U5 enabled EXTI pending status and acknowledgement.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#include "hal_gpio_int.h"

uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return 0U;
#if (HAL_GPIO_INT_STM32_HAS_EXTI == 1U)
    return (EXTI->RPR1 | EXTI->FPR1) & EXTI->IMR1 & (uint32_t)GPIO_Pin;
#else
    (void)GPIO_Pin;
    return 0U;
#endif
}

void HAL_GPIO_ClearInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return;
#if (HAL_GPIO_INT_STM32_HAS_EXTI == 1U)
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
#else
    (void)GPIO_Pin;
#endif
}

uint32_t Platform_GPIO_GetEnabledInterruptStatus(const platform_gpio_t *gpio)
{
    return (gpio == 0) ? 0U : HAL_GPIO_GetEnabledInterruptStatus(gpio->port, gpio->pin);
}

void Platform_GPIO_ClearInterruptStatus(const platform_gpio_t *gpio)
{
    if (gpio != 0) HAL_GPIO_ClearInterruptStatus(gpio->port, gpio->pin);
}
