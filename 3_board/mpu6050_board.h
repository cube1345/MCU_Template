/**
 * @file    mpu6050_board.h
 * @brief   MPU6050 板级绑定层头文件
 * @author  cube
 * @date    2026-04
 */

#ifndef MPU6050_BOARD_H
#define MPU6050_BOARD_H

#include "../2_device/mpu6050_driver.h"

/** @brief 板级初始化：初始化 I2C 并实例化 MPU6050 设备对象 */
void MPU6050_Board_Init(void);
/** @brief 获取 MPU6050 设备对象指针 */
mpu6050_t *MPU6050_Board_GetDevice(void);
/** @brief 获取 MPU6050 的 I2C 器件地址 */
uint8_t MPU6050_Board_GetDeviceAddr(void);

#endif
