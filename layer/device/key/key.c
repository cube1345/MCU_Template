/* SPDX-License-Identifier: MIT */
/**
 * @file    key.c
 * @brief   按键基础对象绑定与操作分发。
 * @author  weixin
 */
#include "key.h"

#include <stddef.h>

/**
 * @brief  检查按键对象是否已经完成绑定。
 * @param  self 按键对象。
 * @return true 表示对象可用，false 表示对象无效或未初始化。
 */
static bool key_is_ready(const struct key *self)
{
  return (self != NULL) && self->initialized && (self->ops != NULL);
}

/**
 * @brief  初始化按键基础对象并绑定具体驱动。
 * @param  self 调用者提供的按键对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  ops 具体驱动操作表，is_pressed 为必选能力。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不直接访问硬件。
 */
int key_init(struct key *self, const char *name,
             const struct key_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (ops == NULL) ||
      (ops->is_pressed == NULL)) {
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
 * @brief  读取按键当前是否处于按下状态。
 * @param  self 已初始化的按键对象。
 * @param  pressed 用于接收按下状态的输出参数。
 * @return PLATFORM_OK 表示读取成功；其他 PLATFORM_* 表示失败。
 */
int key_is_pressed(struct key *self, bool *pressed)
{
  if (pressed == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!key_is_ready(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return self->ops->is_pressed(self, pressed);
}

/**
 * @brief  获取按键调试名称。
 * @param  self 按键对象。
 * @return 按键名称；对象无效时返回 "unknown"。
 */
const char *key_get_name(const struct key *self)
{
  return key_is_ready(self) ? self->name : "unknown";
}
