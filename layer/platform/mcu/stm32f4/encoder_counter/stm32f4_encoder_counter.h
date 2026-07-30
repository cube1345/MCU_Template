/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_encoder_counter.h
 * @brief   STM32F4硬件正交编码器计数器创建接口。
 * @author  weixin
 */
#ifndef STM32F4_ENCODER_COUNTER_H
#define STM32F4_ENCODER_COUNTER_H

#include "platform/inferface/encoder_counter/encoder_counter.h"

#include <stdint.h>

struct stm32f4_encoder_counter_config {
  int32_t id;
  const char *name;
  int32_t pin_a;
  int32_t pin_b;
  uint32_t input_filter;
};

/**
 * @brief  创建或获取一个静态STM32F4编码器计数器对象。
 * @param  config 定时器编号、A/B相引脚和数字滤波配置。
 * @return 成功时返回Platform计数器对象，配置无效时返回NULL。
 * @note   不使用堆内存；相同编号只能绑定一组固定配置。
 */
struct encoder_counter *stm32f4_encoder_counter_create(
    const struct stm32f4_encoder_counter_config *config);

#endif /* STM32F4_ENCODER_COUNTER_H */
