#ifndef MYUSED_MYBOARDS_OLED_BOARD_H
#define MYUSED_MYBOARDS_OLED_BOARD_H

#include "../2_device/OLED_driver.h"

void OLED_Driver_Init(void);
void OLED_Board_Init(void);
/** @brief Select a caller-owned display for legacy Facade calls; NULL unbinds. */
void OLED_Board_Bind(oled_t *device);
oled_t *OLED_Board_GetDevice(void);

#endif
