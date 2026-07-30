/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_format.h
 * @brief   单色画布的数字、浮点数和格式化文字绘制接口。
 * @author  weixin
 */
#ifndef MONO_TEXT_FORMAT_H
#define MONO_TEXT_FORMAT_H

#include "graphics/mono_canvas.h"

#include <stdint.h>

struct mono_number_format {
  int16_t x;
  int16_t y;
  uint8_t digits;
  uint8_t text_size;
};

struct mono_float_format {
  int16_t x;
  int16_t y;
  uint8_t integer_digits;
  uint8_t fraction_digits;
  uint8_t text_size;
};

/**
 * @brief  按固定数字位数绘制无符号十进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、数字位数和字体尺寸。
 * @param  value 要显示的数值。
 * @return PLATFORM_OK表示成功；位数不足或参数无效时返回对应错误。
 */
int mono_text_draw_u32(struct mono_canvas *self,
                       const struct mono_number_format *format,
                       uint32_t value);

/**
 * @brief  按符号加固定数字位数绘制有符号十进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、绝对值数字位数和字体尺寸。
 * @param  value 要显示的数值，包括INT32_MIN。
 * @return PLATFORM_OK表示成功；位数不足或参数无效时返回对应错误。
 */
int mono_text_draw_i32(struct mono_canvas *self,
                       const struct mono_number_format *format,
                       int32_t value);

/**
 * @brief  按固定数字位数绘制大写十六进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、数字位数和字体尺寸。
 * @param  value 要显示的数值。
 * @return PLATFORM_OK表示成功；位数不足或参数无效时返回对应错误。
 */
int mono_text_draw_hex_u32(struct mono_canvas *self,
                           const struct mono_number_format *format,
                           uint32_t value);

/**
 * @brief  按固定数字位数绘制二进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、数字位数和字体尺寸。
 * @param  value 要显示的数值。
 * @return PLATFORM_OK表示成功；位数不足或参数无效时返回对应错误。
 */
int mono_text_draw_bin_u32(struct mono_canvas *self,
                           const struct mono_number_format *format,
                           uint32_t value);

/**
 * @brief  按固定整数位和小数位绘制带符号浮点数。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、整数位、小数位和字体尺寸。
 * @param  value 要显示的有限浮点数。
 * @return PLATFORM_OK表示成功；数值非有限或位数不足时返回对应错误。
 */
int mono_text_draw_f64(struct mono_canvas *self,
                       const struct mono_float_format *format,
                       double value);

/**
 * @brief  使用有界缓冲区格式化并绘制ASCII字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text_size MONO_TEXT_ASCII_6X8或MONO_TEXT_ASCII_8X16。
 * @param  format printf格式字符串，生命周期覆盖本次调用。
 * @return PLATFORM_OK表示成功；格式化溢出或绘制失败时返回对应错误。
 * @note   内部缓冲区固定为64字节，不使用堆；格式和参数必须匹配，
 *         不支持在中断中并发修改同一画布。
 */
int mono_text_printf(struct mono_canvas *self,
                     int16_t x,
                     int16_t y,
                     uint8_t text_size,
                     const char *format,
                     ...);

#endif /* MONO_TEXT_FORMAT_H */
