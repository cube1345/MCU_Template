#ifndef ICM42688_BOARD_H
#define ICM42688_BOARD_H

#include "../2_device/icm42688_driver.h"

void ICM42688_Board_Init(void);
icm42688_t *ICM42688_Board_GetDevice(void);
uint8_t ICM42688_Board_GetDeviceAddr(void);

#endif
