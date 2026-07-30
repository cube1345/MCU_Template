/**
 * @file    board_led.h
 * @brief   板级 LED 实例组合和抽象句柄获取接口。
 * @author  weixin
 */
#ifndef BOARD_LED_H
#define BOARD_LED_H

#include "led.h"

/**
 * @brief  初始化板级 LED 对象组合。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化或 LED 后端绑定失败。
 * @note   应用层只能通过 struct led 和 led_* 接口操作 LED。
 */
int board_led_init(void);

/**
 * @brief  获取红色 LED 抽象对象。
 * @return 初始化成功后返回 struct led *，否则返回 NULL。
 */
struct led *board_led_red_get(void);

/**
 * @brief  获取蓝色 LED 抽象对象。
 * @return 初始化成功后返回 struct led *，否则返回 NULL。
 */
struct led *board_led_blue_get(void);

#endif /* BOARD_LED_H */
