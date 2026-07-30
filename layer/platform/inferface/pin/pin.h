/* SPDX-License-Identifier: MIT */
/**
 * @file    pin.h
 * @brief   平台数字引脚抽象接口。
 * @author  weixin
 *
 * MCU 启动时注册一张静态 ops 表。
 * 调用者只使用整数引脚编号，不接触 HAL 类型。
 * mode 回调负责完成引脚硬件初始化。
 */
#ifndef PIN_H
#define PIN_H

#include <stdint.h>

#define PIN_LOW INT32_C(0)
#define PIN_HIGH INT32_C(1)

#define PIN_MODE_OUTPUT INT32_C(0)
#define PIN_MODE_INPUT INT32_C(1)
#define PIN_MODE_INPUT_PULLUP INT32_C(2)

struct pin_ops {
  void (*mode)(int32_t pin, int32_t mode);
  void (*write)(int32_t pin, int32_t value);
  int32_t (*read)(int32_t pin);
  int32_t (*get)(const char *name);
};

/**
 * @brief  注册当前 MCU 的数字引脚后端。
 * @param  ops MCU 后端提供的引脚操作表。
 * @return 0 表示注册成功，-1 表示参数无效。
 */
int pin_register(const struct pin_ops *ops);

/**
 * @brief  配置平台引脚模式。
 * @param  pin 平台引脚编号。
 * @param  mode PIN_MODE_* 模式值。
 * @return 无返回值。
 */
void pin_mode(int32_t pin, int32_t mode);

/**
 * @brief  写入平台引脚电平。
 * @param  pin 平台引脚编号。
 * @param  value PIN_LOW 或 PIN_HIGH。
 * @return 无返回值。
 */
void pin_write(int32_t pin, int32_t value);

/**
 * @brief  读取平台引脚电平。
 * @param  pin 平台引脚编号。
 * @return PIN_LOW、PIN_HIGH，或后端定义的错误值。
 */
int32_t pin_read(int32_t pin);

/**
 * @brief  通过名称查询平台引脚编号。
 * @param  name MCU 后端支持的引脚名称，例如 "PB5"。
 * @return 平台引脚编号；查询失败时返回 -1。
 */
int32_t pin_get(const char *name);

#endif /* PIN_H */
