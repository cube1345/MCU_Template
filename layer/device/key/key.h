/* SPDX-License-Identifier: MIT */
/**
 * @file    key.h
 * @brief   按键基础设备及统一读取接口。
 * @author  weixin
 *
 * Key 是简单设备，只表达“当前是否按下”。消抖、短按、长按和连续触发属于
 * 输入处理策略，由 component/key_event 提供。
 */
#ifndef KEY_H
#define KEY_H

#include "result.h"

#include <stdbool.h>

struct key;

struct key_ops {
  int (*is_pressed)(struct key *self, bool *pressed);
};

struct key {
  const struct key_ops *ops;
  const char *name;
  bool initialized;
};

/**
 * @brief  初始化按键基础对象并绑定具体驱动。
 * @param  self 调用者提供的按键对象。
 * @param  name 调试名称，生命周期必须长于 self。
 * @param  ops 具体驱动操作表，is_pressed 为必选能力。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不直接访问硬件。
 */
int key_init(struct key *self, const char *name,
             const struct key_ops *ops);

/**
 * @brief  读取按键当前是否处于按下状态。
 * @param  self 已初始化的按键对象。
 * @param  pressed 用于接收按下状态的输出参数。
 * @return PLATFORM_OK 表示读取成功；其他 PLATFORM_* 表示失败。
 */
int key_is_pressed(struct key *self, bool *pressed);

/**
 * @brief  获取按键调试名称。
 * @param  self 按键对象。
 * @return 按键名称；对象无效时返回 "unknown"。
 */
const char *key_get_name(const struct key *self);

#endif /* KEY_H */
