/* SPDX-License-Identifier: MIT */
/**
 * @file    pwm.c
 * @brief   PWM 全局 ops 分发器。
 * @author  weixin
 */
#include "platform/inferface/pwm/pwm.h"

#include "result.h"

#include <stddef.h>

static const struct pwm_ops *g_ops = NULL;

/**
 * @brief  注册当前 MCU 的 PWM 后端操作表。
 * @param  ops MCU 后端提供的 PWM 操作表。
 * @return 0 表示注册成功，-1 表示参数无效。
 * @note   当前工程只使用一个 MCU 定时器后端。
 */
int pwm_register(const struct pwm_ops *ops)
{
  if ((ops == NULL) || (ops->enable == NULL) ||
      (ops->disable == NULL) || (ops->set_duty == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((g_ops != NULL) && (g_ops != ops)) {
    return PLATFORM_ERR_BUSY;
  }

  g_ops = ops;
  return PLATFORM_OK;
}

/**
 * @brief  启动指定 PWM 通道。
 * @param  channel 平台 PWM 通道编号。
 * @return 0 表示成功，-1 表示后端未注册或能力缺失。
 */
int pwm_enable(int32_t channel)
{
  if (g_ops == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return g_ops->enable(channel);
}

/**
 * @brief  停止指定 PWM 通道。
 * @param  channel 平台 PWM 通道编号。
 * @return 0 表示成功，-1 表示后端未注册或能力缺失。
 */
int pwm_disable(int32_t channel)
{
  if (g_ops == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return g_ops->disable(channel);
}

/**
 * @brief  设置指定 PWM 通道占空比。
 * @param  channel 平台 PWM 通道编号。
 * @param  duty 万分比占空比，范围为 0..10000。
 * @return 0 表示成功，-1 表示后端未注册或能力缺失。
 */
int pwm_set_duty(int32_t channel, uint16_t duty)
{
  if (g_ops == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if (duty > PLATFORM_PWM_DUTY_MAX) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  return g_ops->set_duty(channel, duty);
}
