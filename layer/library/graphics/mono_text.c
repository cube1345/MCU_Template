/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text.c
 * @brief   单色画布固定宽度字符和字符串绘制实现。
 * @author  weixin
 */
#include "graphics/mono_text.h"

#include "result.h"

#include <stdbool.h>
#include <stddef.h>

#define MONO_TEXT_BITS_PER_BYTE UINT32_C(8)

/**
 * @brief  计算字体每行占用的字节数。
 * @param  font 字体对象。
 * @return 每行字节数；字体为空时返回0。
 */
static uint32_t mono_text_bytes_per_row(
    const struct mono_font *font)
{
  if (font == NULL) {
    return 0U;
  }

  return ((uint32_t)font->width + MONO_TEXT_BITS_PER_BYTE - 1U) /
         MONO_TEXT_BITS_PER_BYTE;
}

/**
 * @brief  检查字体元数据和字模存储空间是否一致。
 * @param  font 字体对象。
 * @return true表示字体有效，false表示无效。
 */
static bool mono_text_font_is_valid(const struct mono_font *font)
{
  uint32_t glyph_count;
  uint32_t glyph_size;
  uint32_t required_size;

  if ((font == NULL) || (font->data == NULL) ||
      (font->width == 0U) || (font->height == 0U) ||
      (font->first_char > font->last_char)) {
    return false;
  }

  glyph_count = (uint32_t)font->last_char - font->first_char + 1U;
  glyph_size = mono_text_bytes_per_row(font) * font->height;
  required_size = glyph_count * glyph_size;
  return (glyph_size != 0U) && (font->data_size >= required_size);
}

/**
 * @brief  检查文字样式是否包含有效字体和目标尺寸。
 * @param  style 文字样式。
 * @return true表示有效，false表示无效。
 */
static bool mono_text_style_is_valid(
    const struct mono_text_style *style)
{
  return (style != NULL) && mono_text_font_is_valid(style->font) &&
         (style->width > 0U) && (style->height > 0U) &&
         (style->width <= INT16_MAX) &&
         (style->height <= INT16_MAX);
}

/**
 * @brief  检查字符是否包含在指定字体中。
 * @param  font 已通过元数据校验的字体。
 * @param  character 待检查字符。
 * @return true表示支持，false表示不支持。
 */
static bool mono_text_char_is_supported(
    const struct mono_font *font,
    char character)
{
  uint8_t code;

  code = (uint8_t)character;
  return (code >= font->first_char) && (code <= font->last_char);
}

/**
 * @brief  获取字符对应的字模起始地址。
 * @param  font 已通过校验的字体。
 * @param  character 字体支持的字符。
 * @return 字模只读起始地址。
 */
static const uint8_t *mono_text_glyph_get(
    const struct mono_font *font,
    char character)
{
  uint32_t glyph_index;
  uint32_t glyph_size;

  glyph_index = (uint32_t)(uint8_t)character - font->first_char;
  glyph_size = mono_text_bytes_per_row(font) * font->height;
  return &font->data[glyph_index * glyph_size];
}

/**
 * @brief  判断字模指定像素是否点亮。
 * @param  glyph 字模起始地址。
 * @param  bytes_per_row 字模每行字节数。
 * @param  column 像素列号。
 * @param  row 像素行号。
 * @return true表示点亮，false表示熄灭。
 */
static bool mono_text_glyph_pixel_get(const uint8_t *glyph,
                                      uint32_t bytes_per_row,
                                      uint16_t column,
                                      uint16_t row)
{
  uint32_t index;
  uint8_t mask;

  index = (uint32_t)row * bytes_per_row +
          ((uint32_t)column / MONO_TEXT_BITS_PER_BYTE);
  mask = (uint8_t)(UINT8_C(1) <<
                   ((uint32_t)column % MONO_TEXT_BITS_PER_BYTE));
  return (glyph[index] & mask) != 0U;
}

/**
 * @brief  在单色画布中绘制一个固定宽度字符。
 * @param  self 已初始化的画布对象。
 * @param  x 字符左上角横坐标。
 * @param  y 字符左上角纵坐标。
 * @param  character 要绘制的字符。
 * @param  font 只读字体对象。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_char_styled(
    struct mono_canvas *self,
    int16_t x,
    int16_t y,
    char character,
    const struct mono_text_style *style)
{
  struct mono_rect area;
  const uint8_t *glyph;
  const struct mono_font *font;
  uint32_t bytes_per_row;
  uint16_t source_column;
  uint16_t source_row;
  uint16_t column;
  uint16_t row;
  bool on;
  int result;

  if (!mono_text_style_is_valid(style)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  font = style->font;
  if (!mono_text_char_is_supported(font, character)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  area.x = x;
  area.y = y;
  area.width = style->width;
  area.height = style->height;
  result = mono_canvas_draw_rect(self, &area, true, false);
  if (result != PLATFORM_OK) {
    return result;
  }

  glyph = mono_text_glyph_get(font, character);
  bytes_per_row = mono_text_bytes_per_row(font);
  for (row = 0U; row < style->height; row++) {
    source_row = (uint16_t)(((uint32_t)row * font->height) /
                            style->height);
    for (column = 0U; column < style->width; column++) {
      source_column = (uint16_t)(
          ((uint32_t)column * font->width) / style->width);
      on = mono_text_glyph_pixel_get(glyph, bytes_per_row,
                                     source_column, source_row);
      if (on) {
        (void)mono_canvas_draw_pixel(
            self, (int16_t)(x + (int16_t)column),
            (int16_t)(y + (int16_t)row), true);
      }
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  在单色画布中按字体原始尺寸绘制一个字符。
 * @param  self 已初始化的画布对象。
 * @param  x 字符左上角横坐标。
 * @param  y 字符左上角纵坐标。
 * @param  character 要绘制的字符。
 * @param  font 只读字体对象。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_char(struct mono_canvas *self,
                        int16_t x,
                        int16_t y,
                        char character,
                        const struct mono_font *font)
{
  struct mono_text_style style;

  if (font == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  style.font = font;
  style.width = font->width;
  style.height = font->height;
  return mono_text_draw_char_styled(self, x, y, character, &style);
}

/**
 * @brief  校验字符串字符范围并计算字符数量。
 * @param  text 以空字符结尾的字符串。
 * @param  font 已通过校验的字体。
 * @param  length 用于接收字符数量。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_string_length_get(const char *text,
                                       const struct mono_font *font,
                                       uint16_t *length)
{
  uint32_t count;

  if ((text == NULL) || (length == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  count = 0U;
  while (text[count] != '\0') {
    if (!mono_text_char_is_supported(font, text[count])) {
      return PLATFORM_ERR_UNSUPPORTED;
    }
    count++;
    if (count > UINT16_MAX) {
      return PLATFORM_ERR_INVALID_ARGUMENT;
    }
  }

  *length = (uint16_t)count;
  return PLATFORM_OK;
}

/**
 * @brief  在单色画布中绘制固定宽度字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text 以空字符结尾的字符串。
 * @param  font 只读字体对象。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_string_styled(
    struct mono_canvas *self,
    int16_t x,
    int16_t y,
    const char *text,
    const struct mono_text_style *style)
{
  struct mono_rect area;
  const struct mono_font *font;
  uint16_t length;
  uint16_t index;
  uint32_t width;
  int result;

  if (!mono_text_style_is_valid(style)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  font = style->font;

  result = mono_text_string_length_get(text, font, &length);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (length == 0U) {
    return PLATFORM_OK;
  }

  width = (uint32_t)length * style->width;
  if (width > UINT16_MAX) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  area.x = x;
  area.y = y;
  area.width = (uint16_t)width;
  area.height = style->height;
  result = mono_canvas_draw_rect(self, &area, true, false);
  if (result != PLATFORM_OK) {
    return result;
  }

  for (index = 0U; index < length; index++) {
    result = mono_text_draw_char_styled(
        self, (int16_t)(x + (int16_t)(index * style->width)),
        y, text[index], style);
    if (result != PLATFORM_OK) {
      return result;
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  在单色画布中按字体原始尺寸绘制字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text 以空字符结尾的字符串。
 * @param  font 只读字体对象。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_string(struct mono_canvas *self,
                          int16_t x,
                          int16_t y,
                          const char *text,
                          const struct mono_font *font)
{
  struct mono_text_style style;

  if (font == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  style.font = font;
  style.width = font->width;
  style.height = font->height;
  return mono_text_draw_string_styled(self, x, y, text, &style);
}
