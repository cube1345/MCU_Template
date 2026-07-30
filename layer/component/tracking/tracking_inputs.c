/* SPDX-License-Identifier: MIT */
/**
 * @file    tracking_inputs.c
 * @brief   八路数字循迹输入采样组件实现。
 * @author  weixin
 */
#include "tracking_inputs.h"

#include "platform/inferface/pin/pin.h"
#include "result.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief  校验八路循迹输入配置中的引脚编号和重复项。
 * @param  config 待校验的引脚配置。
 * @return true 表示配置有效，false 表示存在负数或重复引脚。
 */
static bool tracking_inputs_config_is_valid(
    const struct tracking_inputs_config *config)
{
  uint8_t current;
  uint8_t previous;

  if (config == NULL) {
    return false;
  }

  for (current = 0U; current < TRACKING_INPUTS_COUNT; current++) {
    if (config->pins[current] < 0) {
      return false;
    }
    for (previous = 0U; previous < current; previous++) {
      if (config->pins[current] == config->pins[previous]) {
        return false;
      }
    }
  }
  return true;
}

/**
 * @brief  初始化八路循迹输入组件并配置所有 GPIO。
 * @param  self 调用者持有的组件对象。
 * @param  config 生命周期仅需覆盖本次调用的八路引脚配置。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   调用前必须先注册 Platform Pin 后端；GPIO 配置为内部上拉输入。
 */
int tracking_inputs_init(
    struct tracking_inputs *self,
    const struct tracking_inputs_config *config)
{
  uint8_t channel;

  if ((self == NULL) || !tracking_inputs_config_is_valid(config)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  memcpy(&self->config, config, sizeof(self->config));
  for (channel = 0U; channel < TRACKING_INPUTS_COUNT; channel++) {
    pin_mode(self->config.pins[channel], PIN_MODE_INPUT_PULLUP);
  }
  self->initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  读取一个循迹输入通道的原始电平。
 * @param  self 已初始化的组件对象。
 * @param  channel 通道编号，范围为 0 至 TRACKING_INPUTS_COUNT - 1。
 * @param  high 用于接收原始电平，true 表示高电平。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   本函数不修改组件状态，不执行滤波或有效电平反转。
 */
int tracking_inputs_read_channel(
    const struct tracking_inputs *self,
    uint8_t channel,
    bool *high)
{
  if ((self == NULL) || (high == NULL) ||
      (channel >= TRACKING_INPUTS_COUNT)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *high = (pin_read(self->config.pins[channel]) == PIN_HIGH);
  return PLATFORM_OK;
}

/**
 * @brief  顺序读取全部八路循迹输入。
 * @param  self 已初始化的组件对象。
 * @param  state 用于接收电平数组和高电平位掩码。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   high_mask 的 bit0 至 bit7 对应通道 0 至 7；采样不是硬件同时锁存。
 */
int tracking_inputs_sample(
    const struct tracking_inputs *self,
    struct tracking_inputs_state *state)
{
  uint8_t channel;

  if ((self == NULL) || (state == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  state->high_mask = UINT8_C(0);
  for (channel = 0U; channel < TRACKING_INPUTS_COUNT; channel++) {
    state->high[channel] =
        (pin_read(self->config.pins[channel]) == PIN_HIGH);
    if (state->high[channel]) {
      state->high_mask = (uint8_t)(
          state->high_mask | (uint8_t)(UINT8_C(1) << channel));
    }
  }
  return PLATFORM_OK;
}
