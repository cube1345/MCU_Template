/**
 * @file    oled_board.h
 * @brief   OLED 板级绑定层头文件
 * @author  cube
 * @date    2026-04
 */

#ifndef MYUSED_MYBOARDS_OLED_BOARD_H
#define MYUSED_MYBOARDS_OLED_BOARD_H

#include "../2_device/OLED_driver.h"

/** @brief 绑定 OLED 软件 IIC 适配器到 Device 层默认实例。 */
void OLED_Driver_Init(void);
/** @brief OLED 板级初始化：绑定 IIC 传输并初始化显示控制器。 */
void OLED_Board_Init(void);

#endif
