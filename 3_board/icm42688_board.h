/**
 * @file    icm42688_board.h
 * @brief   ICM42688 板级绑定层头文件
 * @author  cube
 * @date    2026-04
 */

#ifndef ICM42688_BOARD_H
#define ICM42688_BOARD_H

#include "../2_device/icm42688_driver.h"

/** @brief 板级初始化：初始化 I2C 并实例化 ICM42688 设备对象 */
void ICM42688_Board_Init(void);
/** @brief 获取 ICM42688 设备对象指针 */
icm42688_t *ICM42688_Board_GetDevice(void);
/** @brief 获取 ICM42688 的 I2C 器件地址 */
uint8_t ICM42688_Board_GetDeviceAddr(void);

#endif
