/**
 * @file    led_pwm.h
 * @brief   基于 Platform PWM 的可调光 LED 具体实现。
 * @author  weixin
 */
#ifndef LED_PWM_H
#define LED_PWM_H

#include "led.h"

#include <stdbool.h>
#include <stdint.h>

struct led_pwm {
  /* 必须是首成员，供 LED 基类向上转型。 */
  struct led base;
  /* Board 层定义通道，Platform 后端解释定时器资源。 */
  int32_t channel;
  /* 最近一次成功写入的亮度百分比。 */
  uint8_t brightness;
  /* 仅由成功的 on/off/toggle 操作更新。 */
  bool is_on;
};

/**
 * @brief  初始化 PWM LED 具体对象。
 * @param  self 调用者提供的 PWM LED 对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  channel 平台层 PWM 通道编号。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   初始化完成后 PWM 输出默认关闭。
 */
int led_pwm_init(struct led_pwm *self, const char *name, int32_t channel);

#endif /* LED_PWM_H */
