/**
 * @file    led_pwm.c
 * @brief   基于 Platform PWM 的可调光 LED 具体实现。
 * @author  weixin
 */
#include "led_pwm.h"

#include "container_of.h"
#include "platform/inferface/pwm/pwm.h"

#include <stddef.h>

#define LED_PWM_DEFAULT_BRIGHTNESS_PERCENT UINT8_C(100)
#define LED_PWM_PERCENT_MAX UINT32_C(100)

/**
 * @brief  将百分比亮度转换为平台 PWM 占空比。
 * @param  percent 亮度百分比，范围为 0..100。
 * @return 平台 PWM 占空比数值。
 */
static uint16_t led_pwm_percent_to_duty(uint8_t percent)
{
  return (uint16_t)(((uint32_t)percent *
                    (uint32_t)PLATFORM_PWM_DUTY_MAX) /
                   LED_PWM_PERCENT_MAX);
}

/**
 * @brief  将当前亮度写入 PWM 通道。
 * @param  self PWM LED 对象。
 * @return PLATFORM_OK 表示写入成功；其他 PLATFORM_* 表示失败。
 */
static int led_pwm_apply(const struct led_pwm *self)
{
  return pwm_set_duty(self->channel,
                      led_pwm_percent_to_duty(self->brightness));
}

/**
 * @brief  PWM LED 的点亮操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_pwm_on(struct led *base)
{
  struct led_pwm *self = container_of(base, struct led_pwm, base);
  int result;

  result = led_pwm_apply(self);
  if (result != 0) {
    return result;
  }
  result = pwm_enable(self->channel);
  if (result == PLATFORM_OK) {
    self->is_on = true;
  }
  return result;
}

/**
 * @brief  PWM LED 的熄灭操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_pwm_off(struct led *base)
{
  struct led_pwm *self = container_of(base, struct led_pwm, base);
  int result;

  result = pwm_set_duty(self->channel, PLATFORM_PWM_DUTY_MIN);
  if (result != 0) {
    return result;
  }
  result = pwm_disable(self->channel);
  if (result == PLATFORM_OK) {
    self->is_on = false;
  }
  return result;
}

/**
 * @brief  PWM LED 的翻转操作。
 * @param  base LED 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_pwm_toggle(struct led *base)
{
  struct led_pwm *self = container_of(base, struct led_pwm, base);

  return self->is_on ? led_pwm_off(base) : led_pwm_on(base);
}

/**
 * @brief  设置 PWM LED 亮度。
 * @param  base LED 基类对象。
 * @param  percent 亮度百分比，范围为 0..100。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int led_pwm_set_brightness(struct led *base, uint8_t percent)
{
  struct led_pwm *self = container_of(base, struct led_pwm, base);
  int result;

  result = pwm_set_duty(self->channel,
                        led_pwm_percent_to_duty(percent));
  if (result == PLATFORM_OK) {
    self->brightness = percent;
  }
  return result;
}

static const struct led_ops s_ops = {
  .on = led_pwm_on,
  .off = led_pwm_off,
  .toggle = led_pwm_toggle,
  .set_brightness = led_pwm_set_brightness,
};

/**
 * @brief  初始化 PWM LED 具体对象。
 * @param  self 调用者提供的 PWM LED 对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  channel 平台层 PWM 通道编号。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   初始化完成后 PWM 输出默认关闭。
 */
int led_pwm_init(struct led_pwm *self, const char *name, int32_t channel)
{
  int result;

  if ((self == NULL) || (name == NULL) || (channel < 0)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  self->channel = channel;
  self->brightness = LED_PWM_DEFAULT_BRIGHTNESS_PERCENT;
  self->is_on = false;
  result = led_init(&self->base, name, &s_ops);
  if (result != PLATFORM_OK) {
    return result;
  }
  return led_off(&self->base);
}
