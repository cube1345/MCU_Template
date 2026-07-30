/* SPDX-License-Identifier: MIT */
/**
 * @file    key_gpio.c
 * @brief   GPIO 按键的物理电平读取和有效电平转换。
 * @author  weixin
 */
#include "key_gpio.h"

#include "container_of.h"
#include "platform/inferface/pin/pin.h"

#include <stddef.h>

/**
 * @brief  读取 GPIO 按键并转换为逻辑按下状态。
 * @param  base 按键基类对象。
 * @param  pressed 用于接收逻辑按下状态的输出参数。
 * @return PLATFORM_OK 表示读取成功；其他 PLATFORM_* 表示失败。
 */
static int key_gpio_is_pressed(struct key *base, bool *pressed)
{
  struct key_gpio *self;
  int32_t level;

  if ((base == NULL) || (pressed == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct key_gpio, base);
  level = pin_read(self->pin);
  if ((level != PIN_LOW) && (level != PIN_HIGH)) {
    return PLATFORM_ERR_IO;
  }

  *pressed = self->active_high ? (level == PIN_HIGH) :
                                 (level == PIN_LOW);
  return PLATFORM_OK;
}

static const struct key_ops s_ops = {
  .is_pressed = key_gpio_is_pressed,
};

/**
 * @brief  初始化 GPIO 按键具体对象。
 * @param  self 调用者提供的 GPIO 按键对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  pin 平台层引脚编号。
 * @param  pin_mode_value 平台层输入模式。
 * @param  active_high true 表示高电平按下，false 表示低电平按下。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 */
int key_gpio_init(struct key_gpio *self,
                  const char *name,
                  int32_t pin,
                  int32_t pin_mode_value,
                  bool active_high)
{
  if ((self == NULL) || (name == NULL) || (pin < 0) ||
      ((pin_mode_value != PIN_MODE_INPUT) &&
       (pin_mode_value != PIN_MODE_INPUT_PULLUP))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->pin = pin;
  self->pin_mode = pin_mode_value;
  self->active_high = active_high;
  pin_mode(pin, pin_mode_value);
  return key_init(&self->base, name, &s_ops);
}
