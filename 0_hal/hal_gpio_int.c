#include "hal_gpio_int.h"

uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void) GPIOx;
    return (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET) ? (uint32_t) GPIO_Pin : 0U;
}

void HAL_GPIO_ClearInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void) GPIOx;
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
}
