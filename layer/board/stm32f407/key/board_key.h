/* SPDX-License-Identifier: MIT */
/**
 * @file    board_key.h
 * @brief   STM32F407 板载用户按键对象组合接口。
 * @author  weixin
 */
#ifndef BOARD_KEY_H
#define BOARD_KEY_H

#include "key.h"

/**
 * @brief  初始化 STM32F407 板载用户按键。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化或 GPIO 按键绑定失败。
 */
int board_key_init(void);

/**
 * @brief  获取板载用户按键抽象对象。
 * @return 初始化成功后返回 struct key *，否则返回 NULL。
 */
struct key *board_key_user_get(void);

/**
 * @brief  获取PE2低有效EMM使能切换按键。
 * @return 初始化成功后返回struct key *，否则返回NULL。
 */
struct key *board_key_emm_enable_get(void);

/**
 * @brief  获取PE3低有效系统复位按键。
 * @return 初始化成功后返回struct key *，否则返回NULL。
 */
struct key *board_key_reset_get(void);

#endif /* BOARD_KEY_H */
