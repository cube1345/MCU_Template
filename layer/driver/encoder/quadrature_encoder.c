/* SPDX-License-Identifier: MIT */
/**
 * @file    quadrature_encoder.c
 * @brief   基于Platform硬件计数器的正交编码器具体驱动。
 * @author  weixin
 */
#include "quadrature_encoder.h"

#include "container_of.h"
#include "result.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief  启动具体正交编码器的Platform计数器。
 * @param  base Encoder基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int quadrature_encoder_start(struct encoder *base)
{
  struct quadrature_encoder *self =
      container_of(base, struct quadrature_encoder, base);

  return encoder_counter_start(self->counter);
}

/**
 * @brief  停止具体正交编码器的Platform计数器。
 * @param  base Encoder基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int quadrature_encoder_stop(struct encoder *base)
{
  struct quadrature_encoder *self =
      container_of(base, struct quadrature_encoder, base);

  return encoder_counter_stop(self->counter);
}

/**
 * @brief  读取具体正交编码器累计计数并应用方向配置。
 * @param  base Encoder基础对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int quadrature_encoder_read_count(struct encoder *base,
                                         int32_t *count)
{
  struct quadrature_encoder *self =
      container_of(base, struct quadrature_encoder, base);
  int result;

  result = encoder_counter_read(self->counter, count);
  if ((result != PLATFORM_OK) || !self->reversed) {
    return result;
  }
  if (*count == INT32_MIN) {
    *count = INT32_MAX;
  } else {
    *count = -*count;
  }
  return PLATFORM_OK;
}

/**
 * @brief  复位具体正交编码器累计计数。
 * @param  base Encoder基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int quadrature_encoder_reset(struct encoder *base)
{
  struct quadrature_encoder *self =
      container_of(base, struct quadrature_encoder, base);

  return encoder_counter_reset(self->counter);
}

static const struct encoder_ops s_ops = {
  .start = quadrature_encoder_start,
  .stop = quadrature_encoder_stop,
  .read_count = quadrature_encoder_read_count,
  .reset = quadrature_encoder_reset,
};

/**
 * @brief  初始化正交编码器具体对象并绑定Encoder基础类。
 * @param  self 调用者持有的正交编码器对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  counter Platform硬件正交编码器计数对象。
 * @param  reversed true表示对最终累计计数取反。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int quadrature_encoder_init(
    struct quadrature_encoder *self,
    const char *name,
    struct encoder_counter *counter,
    bool reversed)
{
  int result;

  if ((self == NULL) || (name == NULL) || (counter == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = encoder_bind(&self->base, name, &s_ops);
  if (result != PLATFORM_OK) {
    return result;
  }
  self->counter = counter;
  self->reversed = reversed;
  return PLATFORM_OK;
}

/**
 * @brief  获取正交编码器对应的Encoder基础对象。
 * @param  self 正交编码器具体对象。
 * @return Encoder基础对象；self为NULL时返回NULL。
 */
struct encoder *quadrature_encoder_get_encoder(
    struct quadrature_encoder *self)
{
  return (self != NULL) ? &self->base : NULL;
}
