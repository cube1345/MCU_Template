/**
 * @file    stepper_board.h
 * @brief   双步进电机板级绑定层接口
 * @author  cube
 * @date    2026-05-03
 *
 * 职责:
 * - 声明双电机设备对象并传入 Device 层
 * - 定义 EN/DIR GPIO 引脚初始化
 * - 填充 stepper_hw_cfg_t 数组并注入 Device 层
 * - 托管 STEP_CTRL_TIMER ISR
 */

#ifndef STEPPER_BOARD_H
#define STEPPER_BOARD_H

#include "../2_device/stepper_driver.h"

void Stepper_Board_Init(void);
stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId);

#endif /* STEPPER_BOARD_H */
