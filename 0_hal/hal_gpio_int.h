/**
 * @file    hal_gpio_int.h
 * @brief   HAL 层 GPIO 中断状态接口
 * @author  cube
 * @date    2026-04
 *
 * 封装 TI DriverLib 的 GPIO 中断状态读写。
 * 从 hal_gpio.h 中独立出来，专门服务于 Board 层 ISR 中的中断状态查询和清除。
 */

#ifndef __HAL_GPIO_INT_H
#define __HAL_GPIO_INT_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/**
 * @brief  读取已使能的 GPIO 中断状态
 * @param  GPIOx GPIO 端口寄存器基址
 * @param  GPIO_Pin 引脚掩码
 * @retval 中断状态位掩码
 */
uint32_t HAL_GPIO_GetEnabledInterruptStatus(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);

/**
 * @brief  清除 GPIO 中断状态
 * @param  GPIOx GPIO 端口寄存器基址
 * @param  GPIO_Pin 引脚掩码
 */
void HAL_GPIO_ClearInterruptStatus(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);

#endif
