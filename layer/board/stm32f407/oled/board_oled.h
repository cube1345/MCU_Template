/* SPDX-License-Identifier: MIT */
/**
 * @file    board_oled.h
 * @brief   STM32F407板级OLED对象组合接口。
 * @author  weixin
 */
#ifndef BOARD_OLED_H
#define BOARD_OLED_H

#include "display/display.h"
#include "graphics/mono_canvas.h"

/**
 * @brief  初始化连接在I2C1上的SSD1306 OLED。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 * @note   本函数会执行阻塞I2C事务，只能在任务上下文调用。
 */
int board_oled_init(void);

/**
 * @brief  获取板级OLED的显示设备基类。
 * @return 初始化成功后返回struct display *，否则返回NULL。
 * @note   返回对象由board层静态持有，调用者不得释放。
 */
struct display *board_oled_get(void);

/**
 * @brief  获取板级OLED对应的单色画布。
 * @return 初始化成功后返回画布地址，否则返回NULL。
 * @note   画布不保证并发访问安全，绘图和刷新必须由同一任务串行执行。
 */
struct mono_canvas *board_oled_canvas_get(void);

#endif /* BOARD_OLED_H */
