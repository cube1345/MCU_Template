#include "hal_gpio_int.h"

uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    return DL_GPIO_getEnabledInterruptStatus(GPIOx, GPIO_Pin);
}

void HAL_GPIO_ClearInterruptStatus(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_clearInterruptStatus(GPIOx, GPIO_Pin);
}
