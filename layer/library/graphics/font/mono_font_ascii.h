/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_font_ascii.h
 * @brief   内置8x8可打印ASCII点阵字体。
 * @author  weixin
 */
#ifndef MONO_FONT_ASCII_H
#define MONO_FONT_ASCII_H

#include "graphics/font/mono_font.h"

/**
 * @brief  获取内置8x8可打印ASCII字体。
 * @return 字体只读对象，生命周期覆盖整个程序运行期，不会返回NULL。
 * @note   支持字符范围为空格0x20至波浪号0x7E；函数可在任务中调用，
 *         也可在不访问共享画布的中断上下文中调用。
 */
const struct mono_font *mono_font_ascii_8x8_get(void);

#endif /* MONO_FONT_ASCII_H */
