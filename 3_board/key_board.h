#ifndef KEY_BOARD_H
#define KEY_BOARD_H

#include "../2_device/key_driver.h"

void Key_Board_Init(void);
key_t *Key_Board_GetKeys(void);
uint8_t Key_Board_GetNum(void);

#endif
