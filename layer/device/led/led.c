/**
 * @file    led.c
 * @brief   LED 抽象基类的生命周期、状态检查和操作分派。
 * @author  weixin
 *
 * 基类不拥有硬件资源，也不使用堆内存；具体 GPIO/PWM 实现通过 const
 * 操作表注入，应用只调用本文件提供的统一包装函数。
 */
#include "led.h"

#include <stddef.h>

/**
 * @brief  检查 LED 对象是否已经完成绑定。
 * @param  self LED 对象。
 * @return true 表示对象可用，false 表示对象无效或未初始化。
 */
static bool led_is_ready(const struct led *self)
{
  return (self != NULL) && self->initialized && (self->ops != NULL);
}

/**
 * @brief  初始化 LED 抽象基类并绑定具体后端。
 * @param  self 调用方持有的 LED 对象存储。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  ops 不可变操作表，on/off/toggle 均为必选能力。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不直接访问硬件。
 */
int led_init(struct led *self, const char *name,
             const struct led_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (ops == NULL) ||
      (ops->on == NULL) || (ops->off == NULL) ||
      (ops->toggle == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->ops = ops;
  self->name = name;
  self->initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  打开 LED。
 * @param  self 已初始化的 LED 基类句柄。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int led_on(struct led *self)
{
  if (!led_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return self->ops->on(self);
}

/**
 * @brief  关闭 LED。
 * @param  self 已初始化的 LED 基类句柄。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int led_off(struct led *self)
{
  if (!led_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return self->ops->off(self);
}

/**
 * @brief  翻转 LED 当前开关状态。
 * @param  self 已初始化的 LED 基类句柄。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int led_toggle(struct led *self)
{
  if (!led_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return self->ops->toggle(self);
}

/**
 * @brief  设置 LED 亮度。
 * @param  self 已初始化的 LED 基类句柄。
 * @param  percent 亮度百分比，范围为 0..100。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int led_set_brightness(struct led *self, uint8_t percent)
{
  if (percent > 100U) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!led_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if (self->ops->set_brightness == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return self->ops->set_brightness(self, percent);
}
