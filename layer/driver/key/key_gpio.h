/* SPDX-License-Identifier: MIT */
/**
 * @file    key_gpio.h
 * @brief   基于 Platform Pin 的 GPIO 按键具体实现。
 * @author  weixin
 */
#ifndef KEY_GPIO_H
#define KEY_GPIO_H

#include "key.h"

#include <stdbool.h>
#include <stdint.h>

struct key_gpio {
  /* 必须是首成员，供 Key 基础对象向上转型。 */
  struct key base;
  int32_t pin;
  int32_t pin_mode;
  bool active_high;
};

/**
 * @brief  初始化 GPIO 按键具体对象。
 * @param  self 调用者提供的 GPIO 按键对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  pin 平台层引脚编号。
 * @param  pin_mode 平台层输入模式。
 * @param  active_high true 表示高电平按下，false 表示低电平按下。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 */
int key_gpio_init(struct key_gpio *self,
                  const char *name,
                  int32_t pin,
                  int32_t pin_mode,
                  bool active_high);

#endif /* KEY_GPIO_H */
