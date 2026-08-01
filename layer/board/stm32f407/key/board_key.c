/* SPDX-License-Identifier: MIT */
/**
 * @file    board_key.c
 * @brief   STM32F407 PA0 用户按键的对象组合和板级参数绑定。
 * @author  weixin
 */
#include "board_key.h"

#include "key_gpio.h"
#include "platform/inferface/pin/pin.h"
#include "result.h"
#include "stm32f4.h"

#include <stdbool.h>
#include <stddef.h>

static struct key_gpio s_user_key;
static struct key_gpio s_emm_enable_key;
static struct key_gpio s_reset_key;
static bool s_initialized;

/**
 * @brief  初始化 STM32F407 板载用户按键。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化或 GPIO 按键绑定失败。
 * @note   当前用户按键为 PA0，按下为高电平。
 */
int board_key_init(void)
{
  int result;

  if (s_initialized) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  
  if (result == PLATFORM_OK) {
    /*
     * Board 只描述硬件：PA0/WK_UP、普通输入、高电平按下。
     * 消抖和长按时间由 component/key_event 使用默认策略处理。
     */
    result = key_gpio_init(&s_user_key, "user_key",
                           STM32F4_PIN_PA0, PIN_MODE_INPUT, true);
  }
  if (result == PLATFORM_OK) {
    /* PE2使用内部上拉，按键按下时接地并呈低电平。 */
    result = key_gpio_init(&s_emm_enable_key, "emm_enable_key",
                           STM32F4_PIN_PE2,
                           PIN_MODE_INPUT_PULLUP, false);
  }
  if (result == PLATFORM_OK) {
    /* PE3使用内部上拉，按键按下时接地并呈低电平。 */
    result = key_gpio_init(&s_reset_key, "reset_key",
                           STM32F4_PIN_PE3,
                           PIN_MODE_INPUT_PULLUP, false);
  }
  if (result != PLATFORM_OK) {
    return result;
  }

  s_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  获取板载用户按键抽象对象。
 * @return 初始化成功后返回 struct key *，否则返回 NULL。
 * @note   应用层应通过 key/key_event 接口读取按键状态或事件。
 */
struct key *board_key_user_get(void)
{
  return s_initialized ? &s_user_key.base : NULL;
}

struct key *board_key_emm_enable_get(void)
{
  return s_initialized ? &s_emm_enable_key.base : NULL;
}

struct key *board_key_reset_get(void)
{
  return s_initialized ? &s_reset_key.base : NULL;
}
