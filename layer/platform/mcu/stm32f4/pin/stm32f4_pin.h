/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_pin.h
 * @brief   STM32F4 GPIO 后端注册接口。
 * @author  weixin
 */
#ifndef STM32F4_PIN_H
#define STM32F4_PIN_H

/**
 * @brief  注册 STM32F4 GPIO 后端到 platform pin 子系统。
 * @return 0 表示注册成功，-1 表示注册失败。
 */
int stm32f4_pin_register(void);

#endif /* STM32F4_PIN_H */
