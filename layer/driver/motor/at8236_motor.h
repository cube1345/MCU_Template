/* SPDX-License-Identifier: MIT */
/**
 * @file    at8236_motor.h
 * @brief   基于两路Platform PWM的AT8236有刷电机驱动。
 * @author  weixin
 */
#ifndef AT8236_MOTOR_H
#define AT8236_MOTOR_H

#include "device/motor/motor.h"

#include <stdbool.h>
#include <stdint.h>

struct at8236_motor {
  struct motor base;
  int32_t in1_channel;
  int32_t in2_channel;
  int16_t current_output;
  bool reversed;
};

/**
 * @brief  初始化AT8236具体对象并绑定Motor基础类。
 * @param  self 调用者持有的AT8236对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  in1_channel AT8236 IN1连接的Platform PWM通道。
 * @param  in2_channel AT8236 IN2连接的Platform PWM通道。
 * @param  reversed true表示反转正负输出定义。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数不启动PWM；真实硬件在motor_start()中启动。
 */
int at8236_motor_init(struct at8236_motor *self,
                      const char *name,
                      int32_t in1_channel,
                      int32_t in2_channel,
                      bool reversed);

/**
 * @brief  获取AT8236对应的Motor基础对象。
 * @param  self AT8236具体对象。
 * @return Motor基础对象；self为NULL时返回NULL。
 */
struct motor *at8236_motor_get_motor(struct at8236_motor *self);

#endif /* AT8236_MOTOR_H */
