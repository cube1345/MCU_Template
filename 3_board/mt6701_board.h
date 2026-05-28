/**
 * @file    mt6701_board.h
 * @brief   MT6701 板级绑定层头文件
 * @author  cube
 * @date    2026-05-02
 *
 * 职责: 声明 MT6701_Board_Init()，由应用层调用以完成 GPIO 初始化和设备状态初始化。
 */

#ifndef MT6701_BOARD_H
#define MT6701_BOARD_H

/**
 * @brief MT6701 板级初始化
 *
 * 执行顺序:
 * 1. 初始化 M1/M2 CSN、共享 SCLK、共享 DO 的 GPIO IOMUX
 * 2. 调用 Device 层 MT6701_Init() 初始化内部状态
 */
void MT6701_Board_Init(void);

#endif
