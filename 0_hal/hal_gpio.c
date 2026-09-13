/**
 * @file hal_gpio.c
 * @brief GPIO object operations and safe unavailable bindings.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#include "hal_gpio.h"

void HAL_GPIO_SetPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return;
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
#else
    (void)GPIOx;
    (void)GPIO_Pin;
#endif
}

void HAL_GPIO_ClearPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return;
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
#else
    (void)GPIOx;
    (void)GPIO_Pin;
#endif
}

void HAL_GPIO_InitOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    GPIO_InitTypeDef gpio_init = {0};

    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return;
    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &gpio_init);
#else
    (void)GPIOx;
    (void)GPIO_Pin;
#endif
}

void HAL_GPIO_InitInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    GPIO_InitTypeDef gpio_init = {0};

    if ((GPIOx == 0) || (GPIO_Pin == 0U)) return;
    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &gpio_init);
#else
    (void)GPIOx;
    (void)GPIO_Pin;
#endif
}

void HAL_GPIO_InitPeripheralOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                                   uint32_t alternate)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    GPIO_InitTypeDef gpio_init = {0};

    if ((GPIOx == 0) || (GPIO_Pin == 0U) || (alternate > 15U)) return;
    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(GPIOx, &gpio_init);
#else
    (void)GPIOx;
    (void)GPIO_Pin;
    (void)alternate;
#endif
}

void HAL_GPIO_EnableOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    /* STM32 enables the output in InitOutput/InitPeripheralOutput already. */
    (void)GPIOx;
    (void)GPIO_Pin;
}

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value)
{
    if (Platform_GPIO_IsAvailable(gpio) == 0U) {
        return;
    }
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    HAL_GPIO_WritePin(gpio->port, gpio->pin,
                      (value == 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    (void)gpio;
    (void)value;
#endif
}

void Platform_GPIO_Toggle(const platform_gpio_t *gpio)
{
    if (Platform_GPIO_IsAvailable(gpio) == 0U) {
        return;
    }
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    HAL_GPIO_TogglePin(gpio->port, gpio->pin);
#else
    (void)gpio;
#endif
}

uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio)
{
    if (Platform_GPIO_IsAvailable(gpio) == 0U) {
        return 0U;
    }
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    return (HAL_GPIO_ReadPin(gpio->port, gpio->pin) == GPIO_PIN_SET) ? 1U : 0U;
#else
    (void)gpio;
    return 0U;
#endif
}

void Platform_GPIO_Set(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }
    HAL_GPIO_SetPin(gpio->port, gpio->pin);
}

void Platform_GPIO_Clear(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }
    HAL_GPIO_ClearPin(gpio->port, gpio->pin);
}

uint8_t Platform_GPIO_IsAvailable(const platform_gpio_t *gpio)
{
#if (HAL_GPIO_STM32_HAS_GPIO == 1U)
    return ((gpio != 0) && (gpio->port != 0) && (gpio->pin != 0U)) ? 1U : 0U;
#else
    (void)gpio;
    return 0U;
#endif
}

void Platform_GPIO_InitOutput(const platform_gpio_t *gpio)
{
    if (Platform_GPIO_IsAvailable(gpio) != 0U) HAL_GPIO_InitOutput(gpio->port, gpio->pin);
}

void Platform_GPIO_InitInput(const platform_gpio_t *gpio)
{
    if (Platform_GPIO_IsAvailable(gpio) != 0U) HAL_GPIO_InitInput(gpio->port, gpio->pin);
}

void Platform_GPIO_InitPeripheralOutput(const platform_gpio_t *gpio, uint32_t alternate)
{
    if (Platform_GPIO_IsAvailable(gpio) != 0U) {
        HAL_GPIO_InitPeripheralOutput(gpio->port, gpio->pin, alternate);
    }
}
