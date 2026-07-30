/**
 * @file    led_gpio.c
 * @brief   基于 Platform Pin 的 GPIO LED 具体实现。
 * @author  weixin
 */
#include "led_gpio.h"

#include "container_of.h"
#include "platform/inferface/pin/pin.h"

#include <stddef.h>

/**
 * @brief  按有效电平写入 GPIO LED 状态。
 * @param  self GPIO LED 对象。
 * @param  on true 表示点亮，false 表示熄灭。
 * @return PLATFORM_OK 表示写入完成。
 */
static int led_gpio_write(struct led_gpio *self, bool on)
{
  int32_t level;

  level = (on == self->active_high) ? PIN_HIGH : PIN_LOW;
  pin_write(self->pin, level);
  self->is_on = on;
  return PLATFORM_OK;
}

/**
 * @brief  GPIO LED 的点亮操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_gpio_on(struct led *base)
{
  struct led_gpio *self = container_of(base, struct led_gpio, base);

  return led_gpio_write(self, true);
}

/**
 * @brief  GPIO LED 的熄灭操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_gpio_off(struct led *base)
{
  struct led_gpio *self = container_of(base, struct led_gpio, base);

  return led_gpio_write(self, false);
}

/**
 * @brief  GPIO LED 的翻转操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_gpio_toggle(struct led *base)
{
  struct led_gpio *self = container_of(base, struct led_gpio, base);

  return led_gpio_write(self, !self->is_on);
}

static const struct led_ops s_ops = {
  .on = led_gpio_on,
  .off = led_gpio_off,
  .toggle = led_gpio_toggle,
  .set_brightness = NULL,
};

/**
 * @brief  初始化 GPIO LED 具体对象。
 * @param  self 调用者提供的 GPIO LED 对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  pin 平台层引脚编号。
 * @param  active_high true 表示高电平点亮，false 表示低电平点亮。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 */
int led_gpio_init(struct led_gpio *self, const char *name, int32_t pin,
                  bool active_high)
{
  int result;

  if ((self == NULL) || (name == NULL) || (pin < 0)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  self->pin = pin;
  self->active_high = active_high;
  self->is_on = false;
  pin_mode(pin, PIN_MODE_OUTPUT);
  result = led_init(&self->base, name, &s_ops);
  if (result != PLATFORM_OK) {
    return result;
  }
  return led_off(&self->base);
}
