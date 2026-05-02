#include "hal_gpio.h"

/**
 * @brief 模拟 STM32 HAL_GPIO_ReadPin 的功能
 * @param GPIOx 端口号 (例如 GPIOA, GPIOB)
 * @param GPIO_Pin 引脚掩码 (例如 DL_GPIO_PIN_18)
 * @return uint8_t 0 为低电平, 1 为高电平
 */
uint8_t HAL_GPIO_ReadPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    if (DL_GPIO_readPins(GPIOx, GPIO_Pin) & GPIO_Pin) {
        return 1;
    } else {
        return 0;
    }
}

void HAL_GPIO_WritePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin, uint8_t Value)
{
    if (Value == 0) {
        DL_GPIO_clearPins(GPIOx, GPIO_Pin);
    } else {
        DL_GPIO_setPins(GPIOx, GPIO_Pin);
    }
}

void HAL_GPIO_TogglePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_togglePins(GPIOx, GPIO_Pin);
}

void HAL_GPIO_SetPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_setPins(GPIOx, GPIO_Pin);
}

void HAL_GPIO_ClearPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_clearPins(GPIOx, GPIO_Pin);
}

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value)
{
    if (gpio == 0) {
        return;
    }
    HAL_GPIO_WritePin(gpio->port, gpio->pin, value);
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
    return HAL_GPIO_ReadPin(gpio->port, gpio->pin);
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
