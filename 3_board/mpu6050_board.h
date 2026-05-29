#ifndef MPU6050_BOARD_H
#define MPU6050_BOARD_H

#include "../2_device/mpu6050_driver.h"

void MPU6050_Board_Init(void);
mpu6050_t *MPU6050_Board_GetDevice(void);
uint8_t MPU6050_Board_GetDeviceAddr(void);

#endif
