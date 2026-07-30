/* SPDX-License-Identifier: MIT */
/**
 * @file    pwm.h
 * @brief   平台 PWM 抽象接口。
 * @author  weixin
 *
 * PWM 是 MCU 单例资源。
 * 调用者只使用整数通道和 0..10000 的万分比占空比。
 */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define PLATFORM_PWM_DUTY_MIN UINT16_C(0)
#define PLATFORM_PWM_DUTY_MAX UINT16_C(10000)

struct pwm_ops {
  int (*enable)(int32_t channel);
  int (*disable)(int32_t channel);
  int (*set_duty)(int32_t channel, uint16_t duty);
};

/**
 * @brief  注册当前 MCU 的 PWM 后端。
 * @param  ops MCU 后端提供的 PWM 操作表。
 * @return 0 表示注册成功，-1 表示参数无效。
 */
int pwm_register(const struct pwm_ops *ops);

/**
 * @brief  使能指定 PWM 通道。
 * @param  channel 平台 PWM 通道编号。
 * @return 0 表示成功，-1 表示失败。
 */
int pwm_enable(int32_t channel);

/**
 * @brief  禁用指定 PWM 通道。
 * @param  channel 平台 PWM 通道编号。
 * @return 0 表示成功，-1 表示失败。
 */
int pwm_disable(int32_t channel);

/**
 * @brief  设置指定通道的万分比占空比。
 * @param  channel 平台 PWM 通道编号。
 * @param  duty 万分比占空比，范围为 0..10000。
 * @return 0 表示成功，-1 表示失败。
 */
int pwm_set_duty(int32_t channel, uint16_t duty);

#endif /* PWM_H */
