/* SPDX-License-Identifier: MIT */
/**
 * @file    low_pass_filter.c
 * @brief   一阶低通滤波纯算法实现。
 * @author  weixin
 */
#include "low_pass_filter.h"

#include "math_utils.h"
#include "result.h"

#include <stddef.h>

/**
 * @brief  初始化一阶低通滤波器。
 * @param  self：调用方提供的滤波器对象。
 * @param  alpha：新输入权重，合法范围为 0.0f 至 1.0f。
 * @retval PLATFORM_OK：初始化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：self 为 NULL。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：alpha 非法。
 */
int low_pass_filter_init(struct low_pass_filter *self, float alpha)
{
  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if ((!math_is_finite(alpha)) || (alpha < 0.0f) || (alpha > 1.0f)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  self->alpha = alpha;
  self->output = 0.0f;
  self->initialized = true;
  self->has_output = false;
  return PLATFORM_OK;
}

/**
 * @brief  使用指定值复位滤波输出。
 * @param  self：已经初始化的滤波器对象。
 * @param  value：复位后的滤波输出，必须是有限浮点数。
 * @retval PLATFORM_OK：复位成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未初始化。
 */
int low_pass_filter_reset(struct low_pass_filter *self, float value)
{
  if ((self == NULL) || (!math_is_finite(value))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->output = value;
  self->has_output = true;
  return PLATFORM_OK;
}

/**
 * @brief  输入一个新样本并计算滤波结果。
 * @param  self：已经初始化的滤波器对象。
 * @param  input：当前输入样本，必须是有限浮点数。
 * @param  output：用于接收本次滤波结果的地址。
 * @retval PLATFORM_OK：计算成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未初始化。
 */
int low_pass_filter_update(struct low_pass_filter *self,
                           float input,
                           float *output)
{
  if ((self == NULL) || (output == NULL) || (!math_is_finite(input))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  if (!self->has_output) {
    /* 首个样本直接成为输出，避免默认零值引入无意义的过渡过程。 */
    self->output = input;
    self->has_output = true;
  } else {
    self->output += self->alpha * (input - self->output);
  }

  *output = self->output;
  return PLATFORM_OK;
}

/**
 * @brief  获取当前滤波输出。
 * @param  self：已经初始化并至少更新或复位一次的滤波器对象。
 * @param  output：用于接收当前滤波输出的地址。
 * @retval PLATFORM_OK：读取成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未产生有效输出。
 */
int low_pass_filter_get_output(const struct low_pass_filter *self,
                               float *output)
{
  if ((self == NULL) || (output == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if ((!self->initialized) || (!self->has_output)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *output = self->output;
  return PLATFORM_OK;
}
