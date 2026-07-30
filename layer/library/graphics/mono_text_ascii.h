/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_ascii.h
 * @brief   兼容6x8和8x16尺寸的ASCII文字绘制接口。
 * @author  weixin
 */
#ifndef MONO_TEXT_ASCII_H
#define MONO_TEXT_ASCII_H

#include "graphics/mono_canvas.h"

#include <stdint.h>

#define MONO_TEXT_ASCII_6X8 UINT8_C(6)
#define MONO_TEXT_ASCII_8X16 UINT8_C(8)

/**
 * @brief  使用内置ASCII字体绘制单个字符。
 * @param  self 已初始化的画布对象。
 * @param  x 字符左上角横坐标。
 * @param  y 字符左上角纵坐标。
 * @param  character 可打印ASCII字符。
 * @param  text_size MONO_TEXT_ASCII_6X8或MONO_TEXT_ASCII_8X16。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 * @note   不透明绘制，不刷新设备，不分配内存且不保证线程安全。
 */
int mono_text_ascii_draw_char(struct mono_canvas *self,
                              int16_t x,
                              int16_t y,
                              char character,
                              uint8_t text_size);

/**
 * @brief  使用内置ASCII字体绘制字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text 以空字符结尾的可打印ASCII字符串。
 * @param  text_size MONO_TEXT_ASCII_6X8或MONO_TEXT_ASCII_8X16。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 * @note   字符串不自动换行；不刷新设备且不保证线程安全。
 */
int mono_text_ascii_draw_string(struct mono_canvas *self,
                                int16_t x,
                                int16_t y,
                                const char *text,
                                uint8_t text_size);

#endif /* MONO_TEXT_ASCII_H */
