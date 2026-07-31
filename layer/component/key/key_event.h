/* SPDX-License-Identifier: MIT */
/**
 * @file    key_event.h
 * @brief   简化的单按键事件识别组件。
 * @author  weixin
 *
 * 一个 key_event 对象组合一个 struct key，负责消抖、单击、双击、
 * 长按和重复事件识别。多个按键就创建多个 key_event 对象；事件本身
 * 不再编码按键编号。
 */
#ifndef KEY_EVENT_H
#define KEY_EVENT_H

#include "key.h"

#include <stdbool.h>
#include <stdint.h>

#define KEY_EVENT_DEBOUNCE_MS UINT32_C(20)
#define KEY_EVENT_DOUBLE_CLICK_MS UINT32_C(250)
#define KEY_EVENT_LONG_MS UINT32_C(500)
#define KEY_EVENT_REPEAT_START_MS UINT32_C(1000)
#define KEY_EVENT_REPEAT_INTERVAL_MS UINT32_C(100)

/*
 * 0 表示无事件，正数表示按键事件，负数沿用 PLATFORM_* 错误码。
 * 注意：单击会等待 KEY_EVENT_DOUBLE_CLICK_MS，确认没有第二次短按后返回。
 */
#define KEY_EVENT_NONE PLATFORM_OK
#define KEY_EVENT_CLICK INT32_C(1)
#define KEY_EVENT_LONG INT32_C(2)
#define KEY_EVENT_REPEAT INT32_C(3)
#define KEY_EVENT_DOUBLE_CLICK INT32_C(4)
#define KEY_EVENT_SHORT_RELEASE INT32_C(5)
#define KEY_EVENT_LONG_RELEASE INT32_C(6)

struct key_event {
  struct key *key;
  int32_t state;
  uint32_t pressed_time_ms;
  uint32_t released_time_ms;
  uint32_t repeat_time_ms;
  bool initialized;
};

/**
 * @brief  初始化按键事件识别对象。
 * @param  self 调用者提供的按键事件对象。
 * @param  key 已初始化的基础按键对象，生命周期必须长于 self。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   key_event 不拥有 key 的存储，只保存引用。
 */
int key_event_init(struct key_event *self, struct key *key);

/**
 * @brief  轮询一次按键事件状态机。
 * @param  self 已初始化的按键事件对象。
 * @return KEY_EVENT_NONE、KEY_EVENT_CLICK、KEY_EVENT_DOUBLE_CLICK、
 *         KEY_EVENT_LONG、KEY_EVENT_REPEAT，或负数 PLATFORM_* 错误码。
 * @note   本函数不阻塞，适合由 5~10 ms 周期任务调用。
 */
int key_event_poll(struct key_event *self);

#endif /* KEY_EVENT_H */
