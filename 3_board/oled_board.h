/**
 * @file    oled_board.h
 * @brief   OLED 板级绑定层头文件
 * @author  cube
 * @date    2026-04
 */

#ifndef MYUSED_MYBOARDS_OLED_BOARD_H
#define MYUSED_MYBOARDS_OLED_BOARD_H

#include "../2_device/OLED_driver.h"

/** @brief 初始化 OLED I2C 通信层，注入 SCL/SDA 回调 */
void OLED_Driver_Init(void);
/** @brief OLED 板级初始化：配置回调并初始化显示控制器 */
void OLED_Board_Init(void);

#endif
