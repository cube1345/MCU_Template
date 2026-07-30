/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_format.c
 * @brief   单色画布的安全数字和格式化文字绘制实现。
 * @author  weixin
 */
#include "graphics/mono_text_format.h"

#include "graphics/mono_text_ascii.h"
#include "math_utils.h"
#include "result.h"

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define MONO_TEXT_NUMBER_BUFFER_SIZE UINT32_C(40)
#define MONO_TEXT_PRINTF_BUFFER_SIZE UINT32_C(64)
#define MONO_TEXT_DECIMAL_MAX_DIGITS UINT8_C(10)
#define MONO_TEXT_HEX_MAX_DIGITS UINT8_C(8)
#define MONO_TEXT_BINARY_MAX_DIGITS UINT8_C(32)
#define MONO_TEXT_FLOAT_MAX_FRACTION UINT8_C(9)

static const char s_digit_table[] = "0123456789ABCDEF";

/**
 * @brief  将无符号整数转换为固定长度字符串。
 * @param  buffer 输出缓冲区。
 * @param  capacity 缓冲区容量。
 * @param  value 数值。
 * @param  digits 固定数字位数。
 * @param  base 进制，范围2至16。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_u32_to_string(char *buffer,
                                   uint32_t capacity,
                                   uint32_t value,
                                   uint8_t digits,
                                   uint8_t base)
{
  uint8_t index;
  uint8_t digit;

  if ((buffer == NULL) || (digits == 0U) ||
      (base < 2U) || (base > 16U) ||
      ((uint32_t)digits + 1U > capacity)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  for (index = 0U; index < digits; index++) {
    digit = (uint8_t)(value % base);
    buffer[digits - index - 1U] = s_digit_table[digit];
    value /= base;
  }
  if (value != 0U) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  buffer[digits] = '\0';
  return PLATFORM_OK;
}

/**
 * @brief  校验整数格式的位数和字体选项。
 * @param  format 整数格式。
 * @param  maximum_digits 允许的最大数字位数。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_number_format_validate(
    const struct mono_number_format *format,
    uint8_t maximum_digits)
{
  if ((format == NULL) || (format->digits == 0U) ||
      (format->digits > maximum_digits)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((format->text_size != MONO_TEXT_ASCII_6X8) &&
      (format->text_size != MONO_TEXT_ASCII_8X16)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return PLATFORM_OK;
}

/**
 * @brief  转换并绘制任意受支持进制的无符号整数。
 * @param  self 画布对象。
 * @param  format 整数格式。
 * @param  value 数值。
 * @param  base 进制。
 * @param  maximum_digits 最大数字位数。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_draw_unsigned(
    struct mono_canvas *self,
    const struct mono_number_format *format,
    uint32_t value,
    uint8_t base,
    uint8_t maximum_digits)
{
  char buffer[MONO_TEXT_NUMBER_BUFFER_SIZE];
  int result;

  result = mono_text_number_format_validate(format, maximum_digits);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = mono_text_u32_to_string(
      buffer, sizeof(buffer), value, format->digits, base);
  if (result != PLATFORM_OK) {
    return result;
  }
  return mono_text_ascii_draw_string(
      self, format->x, format->y, buffer, format->text_size);
}

/**
 * @brief  绘制固定长度无符号十进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 整数显示格式。
 * @param  value 数值。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_u32(struct mono_canvas *self,
                       const struct mono_number_format *format,
                       uint32_t value)
{
  return mono_text_draw_unsigned(
      self, format, value, 10U, MONO_TEXT_DECIMAL_MAX_DIGITS);
}

/**
 * @brief  绘制符号和固定长度十进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 整数显示格式。
 * @param  value 数值。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_i32(struct mono_canvas *self,
                       const struct mono_number_format *format,
                       int32_t value)
{
  char buffer[MONO_TEXT_NUMBER_BUFFER_SIZE];
  uint32_t magnitude;
  int result;

  result = mono_text_number_format_validate(
      format, MONO_TEXT_DECIMAL_MAX_DIGITS);
  if (result != PLATFORM_OK) {
    return result;
  }
  buffer[0] = (value < 0) ? '-' : '+';
  magnitude = math_abs_i32(value);
  result = mono_text_u32_to_string(
      &buffer[1], sizeof(buffer) - 1U,
      magnitude, format->digits, 10U);
  if (result != PLATFORM_OK) {
    return result;
  }
  return mono_text_ascii_draw_string(
      self, format->x, format->y, buffer, format->text_size);
}

/**
 * @brief  绘制固定长度大写十六进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 整数显示格式。
 * @param  value 数值。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_hex_u32(struct mono_canvas *self,
                           const struct mono_number_format *format,
                           uint32_t value)
{
  return mono_text_draw_unsigned(
      self, format, value, 16U, MONO_TEXT_HEX_MAX_DIGITS);
}

/**
 * @brief  绘制固定长度二进制整数。
 * @param  self 已初始化的画布对象。
 * @param  format 整数显示格式。
 * @param  value 数值。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_bin_u32(struct mono_canvas *self,
                           const struct mono_number_format *format,
                           uint32_t value)
{
  return mono_text_draw_unsigned(
      self, format, value, 2U, MONO_TEXT_BINARY_MAX_DIGITS);
}

/**
 * @brief  校验浮点格式。
 * @param  format 浮点格式。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_float_format_validate(
    const struct mono_float_format *format)
{
  if ((format == NULL) || (format->integer_digits == 0U) ||
      (format->integer_digits > MONO_TEXT_DECIMAL_MAX_DIGITS) ||
      (format->fraction_digits > MONO_TEXT_FLOAT_MAX_FRACTION)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((format->text_size != MONO_TEXT_ASCII_6X8) &&
      (format->text_size != MONO_TEXT_ASCII_8X16)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return PLATFORM_OK;
}

/**
 * @brief  绘制带符号定长浮点数。
 * @param  self 已初始化的画布对象。
 * @param  format 浮点显示格式。
 * @param  value 有限浮点数。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_f64(struct mono_canvas *self,
                       const struct mono_float_format *format,
                       double value)
{
  char buffer[MONO_TEXT_NUMBER_BUFFER_SIZE];
  uint32_t expected_length;
  int written;
  int result;

  result = mono_text_float_format_validate(format);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (!isfinite(value)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  expected_length = (uint32_t)format->integer_digits + 1U;
  if (format->fraction_digits > 0U) {
    expected_length += (uint32_t)format->fraction_digits + 1U;
  }
  written = snprintf(buffer, sizeof(buffer), "%+0*.*f",
                     (int)expected_length,
                     (int)format->fraction_digits, value);
  if ((written < 0) || ((uint32_t)written != expected_length)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  return mono_text_ascii_draw_string(
      self, format->x, format->y, buffer, format->text_size);
}

/**
 * @brief  使用64字节有界缓冲区格式化并绘制字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text_size ASCII字体尺寸。
 * @param  format printf格式字符串。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_printf(struct mono_canvas *self,
                     int16_t x,
                     int16_t y,
                     uint8_t text_size,
                     const char *format,
                     ...)
{
  char buffer[MONO_TEXT_PRINTF_BUFFER_SIZE];
  va_list arguments;
  int written;

  if (format == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  va_start(arguments, format);
  written = vsnprintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);
  if ((written < 0) || ((uint32_t)written >= sizeof(buffer))) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  return mono_text_ascii_draw_string(self, x, y,
                                     buffer, text_size);
}
