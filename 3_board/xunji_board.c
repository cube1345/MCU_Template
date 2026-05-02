#include "xunji_board.h"
#include "hal_gpio.h"

static xunji_t g_default_xunji;

xunji_t *Xunji_Board_GetDefault(void)
{
    return &g_default_xunji;
}

static uint8_t Xunji0_ReadPin(void)
{
	return HAL_GPIO_ReadPin(XUN_PORT, XUN_JI0_PIN);
}

static uint8_t Xunji1_ReadPin(void)
{
	return HAL_GPIO_ReadPin(XUN_PORT, XUN_JI1_PIN);
}

static uint8_t Xunji2_ReadPin(void)
{
	return HAL_GPIO_ReadPin(XUN_PORT, XUN_JI2_PIN);
}

static uint8_t Xunji3_ReadPin(void)
{
	return HAL_GPIO_ReadPin(XUN_PORT, XUN_JI3_PIN);
}

static uint8_t Xunji4_ReadPin(void)
{
	return HAL_GPIO_ReadPin(XUN_PORT, XUN_JI4_PIN);
}

void Xunji_Board_Init(void)
{
	const Xunji_ReadPin_FP pins[XUNJI_COUNT] = {
		Xunji0_ReadPin,
		Xunji1_ReadPin,
		Xunji2_ReadPin,
		Xunji3_ReadPin,
		Xunji4_ReadPin,
	};

	(void)Xunji_Driver_Bind(pins, XUNJI_COUNT);
}
