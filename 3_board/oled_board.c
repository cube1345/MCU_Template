#include "oled_board.h"

static oled_t *g_board_oled;

void OLED_Board_Bind(oled_t *device)
{
    g_board_oled = device;
}

oled_t *OLED_Board_GetDevice(void)
{
    return g_board_oled;
}

void OLED_Driver_Init(void)
{
    OLED_InitDeviceEx(g_board_oled);
}

void OLED_Board_Init(void)
{
    OLED_Driver_Init();
}
