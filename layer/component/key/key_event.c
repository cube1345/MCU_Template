/* SPDX-License-Identifier: MIT */
/**
 * @file    key_event.c
 * @brief   按键消抖、单击、双击、长按和重复事件状态机。
 * @author  weixin
 */
#include "key_event.h"

#include "platform/inferface/timebase/timebase.h"

#include <stddef.h>

#define KEY_EVENT_STATE_RELEASED INT32_C(0)
#define KEY_EVENT_STATE_DEBOUNCE INT32_C(1)
#define KEY_EVENT_STATE_PRESSED INT32_C(2)
#define KEY_EVENT_STATE_CLICK_WAIT INT32_C(3)
#define KEY_EVENT_STATE_DOUBLE_DEBOUNCE INT32_C(4)
#define KEY_EVENT_STATE_DOUBLE_PRESSED INT32_C(5)
#define KEY_EVENT_STATE_LONG INT32_C(6)
#define KEY_EVENT_STATE_REPEAT INT32_C(7)
#define KEY_EVENT_STATE_SHORT_RELEASE_DEBOUNCE INT32_C(8)
#define KEY_EVENT_STATE_LONG_RELEASE_DEBOUNCE INT32_C(9)

/**
 * @brief  计算两个毫秒时间戳之间的差值。
 * @param  now 当前时间戳，单位 ms。
 * @param  last 上一次记录的时间戳，单位 ms。
 * @return now 与 last 的无符号差值。
 * @note   使用无符号回绕特性，兼容 tick 溢出场景。
 */
static uint32_t elapsed_ms(uint32_t now, uint32_t last)
{
  return (uint32_t)(now - last);
}

/**
 * @brief  处理按键释放态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_released(struct key_event *self, bool pressed, uint32_t now)
{
  if (pressed) {
    self->state = KEY_EVENT_STATE_DEBOUNCE;
    self->pressed_time_ms = now;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理首次按下后的消抖态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_debounce(struct key_event *self, bool pressed, uint32_t now)
{
  if (!pressed) {
    self->state = KEY_EVENT_STATE_RELEASED;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->pressed_time_ms) >= KEY_EVENT_DEBOUNCE_MS) {
    self->state = KEY_EVENT_STATE_PRESSED;
    self->pressed_time_ms = now;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理按键稳定按下态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_LONG、KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_pressed(struct key_event *self, bool pressed, uint32_t now)
{
  if (!pressed) {
    /* The mission UI acts immediately on the debounced release edge. */
    self->state = KEY_EVENT_STATE_SHORT_RELEASE_DEBOUNCE;
    self->released_time_ms = now;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->pressed_time_ms) >= KEY_EVENT_LONG_MS) {
    self->state = KEY_EVENT_STATE_LONG;
    return KEY_EVENT_LONG;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理单击确认等待态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_CLICK、KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 * @note   在双击窗口内再次按下会转入双击消抖态。
 */
static int poll_click_wait(struct key_event *self, bool pressed, uint32_t now)
{
  if (pressed) {
    self->state = KEY_EVENT_STATE_DOUBLE_DEBOUNCE;
    self->pressed_time_ms = now;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->released_time_ms) >=
      KEY_EVENT_DOUBLE_CLICK_MS) {
    self->state = KEY_EVENT_STATE_RELEASED;
    return KEY_EVENT_CLICK;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理第二次按下后的消抖态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_double_debounce(struct key_event *self,
                                bool pressed,
                                uint32_t now)
{
  if (!pressed) {
    self->state = KEY_EVENT_STATE_CLICK_WAIT;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->pressed_time_ms) >= KEY_EVENT_DEBOUNCE_MS) {
    self->state = KEY_EVENT_STATE_DOUBLE_PRESSED;
    self->pressed_time_ms = now;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理第二次稳定按下态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_DOUBLE_CLICK、KEY_EVENT_LONG 或 KEY_EVENT_NONE。
 */
static int poll_double_pressed(struct key_event *self,
                               bool pressed,
                               uint32_t now)
{
  if (!pressed) {
    self->state = KEY_EVENT_STATE_RELEASED;
    return KEY_EVENT_DOUBLE_CLICK;
  }

  if (elapsed_ms(now, self->pressed_time_ms) >= KEY_EVENT_LONG_MS) {
    self->state = KEY_EVENT_STATE_LONG;
    return KEY_EVENT_LONG;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理长按确认后的保持态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_REPEAT、KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_long(struct key_event *self, bool pressed, uint32_t now)
{
  if (!pressed) {
    self->state = KEY_EVENT_STATE_LONG_RELEASE_DEBOUNCE;
    self->released_time_ms = now;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->pressed_time_ms) >=
      KEY_EVENT_REPEAT_START_MS) {
    self->state = KEY_EVENT_STATE_REPEAT;
    self->repeat_time_ms = now;
    return KEY_EVENT_REPEAT;
  }

  return KEY_EVENT_NONE;
}

/**
 * @brief  处理长按重复触发态。
 * @param  self 按键事件对象。
 * @param  pressed 当前物理按键是否按下。
 * @param  now 当前时间戳，单位 ms。
 * @return KEY_EVENT_REPEAT、KEY_EVENT_NONE 或负数 PLATFORM_* 错误码。
 */
static int poll_repeat(struct key_event *self, bool pressed, uint32_t now)
{
  if (!pressed) {
    self->state = KEY_EVENT_STATE_LONG_RELEASE_DEBOUNCE;
    self->released_time_ms = now;
    return KEY_EVENT_NONE;
  }

  if (elapsed_ms(now, self->repeat_time_ms) >=
      KEY_EVENT_REPEAT_INTERVAL_MS) {
    self->repeat_time_ms = now;
    return KEY_EVENT_REPEAT;
  }

  return KEY_EVENT_NONE;
}

static int poll_short_release_debounce(struct key_event *self,
                                       bool pressed,
                                       uint32_t now)
{
  if (pressed) {
    self->state = KEY_EVENT_STATE_PRESSED;
    return KEY_EVENT_NONE;
  }
  if (elapsed_ms(now, self->released_time_ms) >=
      KEY_EVENT_DEBOUNCE_MS) {
    self->state = KEY_EVENT_STATE_RELEASED;
    return KEY_EVENT_SHORT_RELEASE;
  }
  return KEY_EVENT_NONE;
}

static int poll_long_release_debounce(struct key_event *self,
                                      bool pressed,
                                      uint32_t now)
{
  if (pressed) {
    self->state = KEY_EVENT_STATE_LONG;
    return KEY_EVENT_NONE;
  }
  if (elapsed_ms(now, self->released_time_ms) >=
      KEY_EVENT_DEBOUNCE_MS) {
    self->state = KEY_EVENT_STATE_RELEASED;
    return KEY_EVENT_LONG_RELEASE;
  }
  return KEY_EVENT_NONE;
}

/**
 * @brief  初始化按键事件识别对象。
 * @param  self 调用者提供的按键事件对象。
 * @param  key 已初始化的基础按键对象，生命周期必须长于 self。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不分配堆内存，只保存 key 引用。
 */
int key_event_init(struct key_event *self, struct key *key)
{
  bool pressed;
  int result;

  if ((self == NULL) || (key == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = key_is_pressed(key, &pressed);
  if (result != PLATFORM_OK) {
    return result;
  }

  self->key = key;
  self->state = pressed ? KEY_EVENT_STATE_DEBOUNCE :
                          KEY_EVENT_STATE_RELEASED;
  self->pressed_time_ms = time_get_ms();
  self->released_time_ms = 0U;
  self->repeat_time_ms = 0U;
  self->initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  轮询一次按键事件状态机。
 * @param  self 已初始化的按键事件对象。
 * @return KEY_EVENT_* 事件值，或负数 PLATFORM_* 错误码。
 * @note   本函数不阻塞，适合由 5~10 ms 周期任务调用。
 */
int key_event_poll(struct key_event *self)
{
  bool pressed;
  uint32_t now;
  int result;

  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized || (self->key == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = key_is_pressed(self->key, &pressed);
  if (result != PLATFORM_OK) {
    return result;
  }
  now = time_get_ms();

  switch (self->state) {
  case KEY_EVENT_STATE_RELEASED:
    return poll_released(self, pressed, now);
  case KEY_EVENT_STATE_DEBOUNCE:
    return poll_debounce(self, pressed, now);
  case KEY_EVENT_STATE_PRESSED:
    return poll_pressed(self, pressed, now);
  case KEY_EVENT_STATE_CLICK_WAIT:
    return poll_click_wait(self, pressed, now);
  case KEY_EVENT_STATE_DOUBLE_DEBOUNCE:
    return poll_double_debounce(self, pressed, now);
  case KEY_EVENT_STATE_DOUBLE_PRESSED:
    return poll_double_pressed(self, pressed, now);
  case KEY_EVENT_STATE_LONG:
    return poll_long(self, pressed, now);
  case KEY_EVENT_STATE_REPEAT:
    return poll_repeat(self, pressed, now);
  case KEY_EVENT_STATE_SHORT_RELEASE_DEBOUNCE:
    return poll_short_release_debounce(self, pressed, now);
  case KEY_EVENT_STATE_LONG_RELEASE_DEBOUNCE:
    return poll_long_release_debounce(self, pressed, now);
  default:
    self->state = KEY_EVENT_STATE_RELEASED;
    return PLATFORM_ERR_INVALID_STATE;
  }
}
