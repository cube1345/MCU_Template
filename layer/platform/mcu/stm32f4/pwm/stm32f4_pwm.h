/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_pwm.h
 * @brief   STM32F4 PWM 后端注册接口。
 * @author  weixin
 */
#ifndef STM32F4_PWM_H
#define STM32F4_PWM_H

/**
 * @brief  注册 STM32F4 PWM 后端到 platform PWM 子系统。
 * @return 0 表示注册成功，-1 表示注册失败。
 */
int stm32f4_pwm_register(void);

#endif /* STM32F4_PWM_H */
