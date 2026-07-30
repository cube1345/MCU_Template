/* SPDX-License-Identifier: MIT */
/**
 * @file    pin.c
 * @brief   数字引脚全局 ops 分发器。
 * @author  weixin
 */
#include "platform/inferface/pin/pin.h"

#include <assert.h>
#include <stddef.h>

/* 单 MCU 工程只有一套 GPIO 控制器实现；后注册者覆盖前注册者。 */
static const struct pin_ops *g_ops = NULL;

/**
 * @brief  注册 MCU GPIO 后端操作表。
 * @param  ops MCU 后端提供的引脚操作表。
 * @return 0 表示注册成功，-1 表示参数无效。
 * @note   单 MCU 工程只有一套 GPIO 后端，重复注册以后一次为准。
 */
int pin_register(const struct pin_ops *ops)
{
  if (ops == NULL) {
    return -1;
  }

  g_ops = ops;
  return 0;
}

/**
 * @brief  配置平台引脚模式。
 * @param  pin 平台引脚编号。
 * @param  mode PIN_MODE_* 模式值。
 * @return 无返回值。
 * @note   调用前必须已经完成 pin_register()。
 */
void pin_mode(int32_t pin, int32_t mode)
{
  assert((g_ops != NULL) && (g_ops->mode != NULL));
  g_ops->mode(pin, mode);
}

/**
 * @brief  写入平台引脚电平。
 * @param  pin 平台引脚编号。
 * @param  value PIN_LOW 或 PIN_HIGH。
 * @return 无返回值。
 * @note   具体引脚编号和电平校验由 MCU 后端完成。
 */
void pin_write(int32_t pin, int32_t value)
{
  assert((g_ops != NULL) && (g_ops->write != NULL));
  g_ops->write(pin, value);
}

/**
 * @brief  读取平台引脚电平。
 * @param  pin 平台引脚编号。
 * @return PIN_LOW、PIN_HIGH，或后端定义的错误值。
 */
int32_t pin_read(int32_t pin)
{
  assert((g_ops != NULL) && (g_ops->read != NULL));
  return g_ops->read(pin);
}

/**
 * @brief  通过名称查询平台引脚编号。
 * @param  name MCU 后端支持的引脚名称，例如 "PB5"。
 * @return 平台引脚编号；查询失败时返回 -1。
 */
int32_t pin_get(const char *name)
{
  assert(g_ops != NULL);
  if (g_ops->get == NULL) {
    return -1;
  }

  return g_ops->get(name);
}
