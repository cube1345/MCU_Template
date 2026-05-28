#include "hal_gpio.h"

void HAL_GPIO_SetPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

void HAL_GPIO_ClearPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}

void HAL_GPIO_InitOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &gpio_init);
}

void HAL_GPIO_InitInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &gpio_init);
}

void HAL_GPIO_InitPeripheralOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                                   uint32_t alternate)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = GPIO_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(GPIOx, &gpio_init);
}

void HAL_GPIO_EnableOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_InitOutput(GPIOx, GPIO_Pin);
}

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value)
{
    if (gpio == 0) {
        return;
    }
    HAL_GPIO_WritePin(gpio->port, gpio->pin,
                      (value == 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Platform_GPIO_Toggle(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }
    HAL_GPIO_TogglePin(gpio->port, gpio->pin);
}

uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return 0U;
    }
    return (HAL_GPIO_ReadPin(gpio->port, gpio->pin) == GPIO_PIN_SET) ? 1U : 0U;
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
