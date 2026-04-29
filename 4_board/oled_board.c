#include "oled_board.h"
#include "ti_msp_dl_config.h"

static void OLED_SCL_Write(uint8_t state)
{
	if (state)
	{
		DL_GPIO_setPins(OLED_PORT, OLED_SCL0_PIN);
	}
	else
	{
		DL_GPIO_clearPins(OLED_PORT, OLED_SCL0_PIN);
	}
}

static void OLED_SDA_Write(uint8_t state)
{
	if (state)
	{
		DL_GPIO_setPins(OLED_PORT, OLED_SDA0_PIN);
	}
	else
	{
		DL_GPIO_clearPins(OLED_PORT, OLED_SDA0_PIN);
	}
}

void OLED_Driver_Init(void)
{
	OLED_I2C_Init(OLED_SCL_Write, OLED_SDA_Write);
}

void OLED_Board_Init(void)
{
	OLED_Driver_Init();
	OLED_Init();
}
