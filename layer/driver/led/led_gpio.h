/**
 * @file    led_gpio.h
 * @brief   基于 Platform Pin 的 GPIO LED 具体实现。
 * @author  weixin
 */
#ifndef LED_GPIO_H
#define LED_GPIO_H

#include "led.h"

#include <stdbool.h>
#include <stdint.h>

struct led_gpio {
  /* 必须是首成员，供 LED 基类向上转型。 */
  struct led base;
  /* Board 层定义编号，Platform 后端解释物理引脚。 */
  int32_t pin;
  /* true 为高电平点亮，false 为低电平点亮。 */
  bool active_high;
  /* 仅由成功的 on/off/toggle 操作更新。 */
  bool is_on;
};

/**
 * @brief  初始化 GPIO LED 具体对象。
 * @param  self 调用者提供的 GPIO LED 对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  pin 平台层引脚编号。
 * @param  active_high true 表示高电平点亮，false 表示低电平点亮。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   初始化完成后 LED 默认关闭。
 */
int led_gpio_init(struct led_gpio *self, const char *name, int32_t pin,
                  bool active_high);

#endif /* LED_GPIO_H */
