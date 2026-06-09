#include "hal_gpio_int.h"

uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void) GPIOx;
#if (HAL_GPIO_INT_STM32_HAS_EXTI == 1U)
    return (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET) ? (uint32_t) GPIO_Pin : 0U;
#else
    (void)GPIO_Pin;
    return 0U;
#endif
}

void HAL_GPIO_ClearInterruptStatus(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void) GPIOx;
#if (HAL_GPIO_INT_STM32_HAS_EXTI == 1U)
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
#else
    (void)GPIO_Pin;
#endif
}
