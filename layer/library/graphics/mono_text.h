/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text.h
 * @brief   单色画布的固定宽度字符和字符串绘制接口。
 * @author  weixin
 *
 * 本模块只修改画布内存，不刷新显示设备，也不访问HAL或RTOS。
 */
#ifndef MONO_TEXT_H
#define MONO_TEXT_H

#include "graphics/font/mono_font.h"
#include "graphics/mono_canvas.h"

#include <stdint.h>

struct mono_text_style {
  /** 作为像素来源的只读点阵字体。 */
  const struct mono_font *font;
  /** 字符绘制后的目标宽度。 */
  uint16_t width;
  /** 字符绘制后的目标高度。 */
  uint16_t height;
};

/**
 * @brief  使用指定样式在单色画布中绘制一个字符。
 * @param  self 已初始化的画布对象。
 * @param  x 字符左上角横坐标。
 * @param  y 字符左上角纵坐标。
 * @param  character 要绘制的字符。
 * @param  style 字体和目标字符尺寸，生命周期覆盖本次调用。
 * @return PLATFORM_OK表示成功；参数、字体或区域无效时返回对应错误。
 * @note   字模会以最近邻方式缩放；采用不透明绘制，不刷新显示设备，
 *         不分配内存且不保证线程安全。
 */
int mono_text_draw_char_styled(struct mono_canvas *self,
                               int16_t x,
                               int16_t y,
                               char character,
                               const struct mono_text_style *style);

/**
 * @brief  使用指定样式绘制以空字符结尾的字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text 以空字符结尾的字符串。
 * @param  style 字体和目标字符尺寸，生命周期覆盖本次调用。
 * @return PLATFORM_OK表示成功；参数、字体或区域无效时返回对应错误。
 * @note   函数会先校验完整字符串；不换行，不刷新显示设备，
 *         不分配内存且不保证线程安全。
 */
int mono_text_draw_string_styled(
    struct mono_canvas *self,
    int16_t x,
    int16_t y,
    const char *text,
    const struct mono_text_style *style);

/**
 * @brief  在单色画布中绘制一个字符。
 * @param  self 已初始化的画布对象，由调用者持有并保证调用期间有效。
 * @param  x 字符左上角横坐标，单位为像素。
 * @param  y 字符左上角纵坐标，单位为像素。
 * @param  character 要绘制的字符，必须位于字体支持范围内。
 * @param  font 只读字体对象，生命周期必须覆盖本次调用。
 * @return PLATFORM_OK表示成功；字符不受支持时返回
 *         PLATFORM_ERR_UNSUPPORTED；参数、字体或区域无效时返回对应错误。
 * @note   本函数采用不透明绘制，会同时覆盖字符前景和背景；不分配内存，
 *         不保证线程安全，也不会自动刷新显示设备。
 */
int mono_text_draw_char(struct mono_canvas *self,
                        int16_t x,
                        int16_t y,
                        char character,
                        const struct mono_font *font);

/**
 * @brief  在单色画布中绘制一个以空字符结尾的字符串。
 * @param  self 已初始化的画布对象，由调用者持有并保证调用期间有效。
 * @param  x 字符串左上角横坐标，单位为像素。
 * @param  y 字符串左上角纵坐标，单位为像素。
 * @param  text 以空字符结尾的字符串，生命周期必须覆盖本次调用。
 * @param  font 只读固定宽度字体，生命周期必须覆盖本次调用。
 * @return PLATFORM_OK表示成功；存在不支持字符时返回
 *         PLATFORM_ERR_UNSUPPORTED；字符串越界或参数无效时返回对应错误。
 * @note   函数先校验整个字符串再绘制，因此校验失败不会产生半截文字；
 *         不支持自动换行，不分配内存，不保证线程安全，也不会刷新屏幕。
 */
int mono_text_draw_string(struct mono_canvas *self,
                          int16_t x,
                          int16_t y,
                          const char *text,
                          const struct mono_font *font);

#endif /* MONO_TEXT_H */
