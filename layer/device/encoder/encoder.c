/* SPDX-License-Identifier: MIT */
/**
 * @file    encoder.c
 * @brief   增量式编码器基础对象生命周期与操作分派。
 * @author  weixin
 */
#include "encoder.h"

#include <stddef.h>

/**
 * @brief  检查编码器对象是否已经启动。
 * @param  self 编码器对象。
 * @return PLATFORM_OK表示可用，其他PLATFORM_*表示状态错误。
 */
static int encoder_ready_check(const struct encoder *self)
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
 * @brief  绑定编码器基础对象与具体驱动操作表。
 * @param  self 调用者持有的编码器对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  ops 具体驱动操作表，所有操作均为必选能力。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_bind(struct encoder *self,
                 const char *name,
                 const struct encoder_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (ops == NULL) ||
      (ops->start == NULL) || (ops->stop == NULL) ||
      (ops->read_count == NULL) || (ops->reset == NULL)) {
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
 * @brief  启动编码器硬件采集。
 * @param  self 已完成绑定的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_start(struct encoder *self)
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
 * @brief  停止编码器硬件采集。
 * @param  self 已经启动的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_stop(struct encoder *self)
{
  int result;

  result = encoder_ready_check(self);
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
 * @param  self 已经启动的编码器对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_read_count(struct encoder *self, int32_t *count)
{
  int result;

  if (count == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = encoder_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  return self->ops->read_count(self, count);
}

/**
 * @brief  将编码器累计计数清零。
 * @param  self 已经启动的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_reset(struct encoder *self)
{
  int result;

  result = encoder_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  return self->ops->reset(self);
}

/**
 * @brief  获取编码器逻辑名称。
 * @param  self 编码器对象。
 * @return 对象名称；对象无效时返回"unknown"。
 */
const char *encoder_get_name(const struct encoder *self)
{
  if ((self == NULL) || (self->name == NULL)) {
    return "unknown";
  }

  return self->name;
}

/**
 * @brief  查询编码器是否已经启动。
 * @param  self 编码器对象。
 * @return true表示已经启动，false表示未启动或对象无效。
 */
bool encoder_is_initialized(const struct encoder *self)
{
  return (self != NULL) && self->initialized;
}
