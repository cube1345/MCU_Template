/* SPDX-License-Identifier: MIT */
/**
 * @file    encoder_counter.c
 * @brief   Platform硬件正交编码器计数接口分派。
 * @author  weixin
 */
#include "encoder_counter.h"

#include <stddef.h>

/**
 * @brief  检查计数器是否处于可操作状态。
 * @param  self 计数器对象。
 * @return PLATFORM_OK表示可用，其他PLATFORM_*表示状态错误。
 */
static int encoder_counter_ready_check(
    const struct encoder_counter *self)
{
  if ((self == NULL) || (self->ops == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return PLATFORM_OK;
}

/**
 * @brief  绑定硬件编码器计数器基础对象与MCU后端。
 * @param  self 调用者持有的计数器基础对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  ops MCU后端操作表，所有操作均为必选能力。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_counter_bind(struct encoder_counter *self,
                         const char *name,
                         const struct encoder_counter_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (ops == NULL) ||
      (ops->start == NULL) || (ops->stop == NULL) ||
      (ops->read == NULL) || (ops->reset == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((self->ops != NULL) || self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->ops = ops;
  self->name = name;
  self->initialized = false;
  return PLATFORM_OK;
}

/**
 * @brief  启动硬件正交编码器计数。
 * @param  self 已完成绑定的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_counter_start(struct encoder_counter *self)
{
  int result;

  if ((self == NULL) || (self->ops == NULL) ||
      (self->ops->start == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_OK;
  }

  result = self->ops->start(self);
  if (result == PLATFORM_OK) {
    self->initialized = true;
  }
  return result;
}

/**
 * @brief  停止硬件正交编码器计数。
 * @param  self 已经启动的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_counter_stop(struct encoder_counter *self)
{
  int result;

  result = encoder_counter_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = self->ops->stop(self);
  if (result == PLATFORM_OK) {
    self->initialized = false;
  }
  return result;
}

/**
 * @brief  读取从最近一次复位开始的有符号累计计数。
 * @param  self 已经启动的计数器对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_counter_read(struct encoder_counter *self, int32_t *count)
{
  int result;

  if (count == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = encoder_counter_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  return self->ops->read(self, count);
}

/**
 * @brief  将硬件与软件累计计数同时清零。
 * @param  self 已经启动的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_counter_reset(struct encoder_counter *self)
{
  int result;

  result = encoder_counter_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  return self->ops->reset(self);
}

/**
 * @brief  查询硬件编码器计数器是否已经启动。
 * @param  self 计数器对象。
 * @return true表示已经启动，false表示未启动或对象无效。
 */
bool encoder_counter_is_initialized(
    const struct encoder_counter *self)
{
  return (self != NULL) && self->initialized;
}
