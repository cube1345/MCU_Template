/* SPDX-License-Identifier: MIT */
/**
 * @file    board_i2c.h
 * @brief   STM32F407 开发板 I2C 资源接口。
 * @author  weixin
 */
#ifndef BOARD_I2C_H
#define BOARD_I2C_H

#include "platform/inferface/i2c/i2c.h"

/**
 * @brief  初始化 STM32F407 板级 I2C1 总线。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化、后端注册或总线获取失败。
 */
int board_i2c1_init(void);

/**
 * @brief  获取板级 I2C1 抽象总线对象。
 * @return 初始化成功后返回 struct i2c *，否则返回 NULL。
 */
struct i2c *board_i2c1_get(void);

#endif /* BOARD_I2C_H */
