/* SPDX-License-Identifier: MIT */
/**
 * @file    quadrature_encoder.h
 * @brief   基于Platform硬件计数器的正交编码器具体驱动。
 * @author  weixin
 */
#ifndef QUADRATURE_ENCODER_H
#define QUADRATURE_ENCODER_H

#include "device/encoder/encoder.h"
#include "platform/inferface/encoder_counter/encoder_counter.h"

#include <stdbool.h>

struct quadrature_encoder {
  struct encoder base;
  struct encoder_counter *counter;
  bool reversed;
};

/**
 * @brief  初始化正交编码器具体对象并绑定Encoder基础类。
 * @param  self 调用者持有的正交编码器对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  counter Platform硬件正交编码器计数对象。
 * @param  reversed true表示对最终累计计数取反。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数只建立软件依赖，真实硬件在encoder_start()中启动。
 */
int quadrature_encoder_init(
    struct quadrature_encoder *self,
    const char *name,
    struct encoder_counter *counter,
    bool reversed);

/**
 * @brief  获取正交编码器对应的Encoder基础对象。
 * @param  self 正交编码器具体对象。
 * @return Encoder基础对象；self为NULL时返回NULL。
 */
struct encoder *quadrature_encoder_get_encoder(
    struct quadrature_encoder *self);

#endif /* QUADRATURE_ENCODER_H */
