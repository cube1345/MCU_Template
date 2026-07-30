/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_multibyte.c
 * @brief   UTF-8或GB2312稀疏字库与ASCII混排实现。
 * @author  weixin
 */
#include "graphics/mono_text_multibyte.h"

#include "graphics/font/mono_font.h"
#include "graphics/mono_text.h"
#include "graphics/mono_text_ascii.h"
#include "result.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define MONO_TEXT_BITS_PER_BYTE UINT32_C(8)
#define MONO_TEXT_ASCII_FIRST UINT8_C(0x20)
#define MONO_TEXT_ASCII_LAST UINT8_C(0x7E)

struct mono_text_token {
  uint8_t bytes[MONO_TEXT_MULTIBYTE_CODE_SIZE];
  uint8_t length;
};

struct mono_text_glyph_data {
  const uint8_t *data;
  uint32_t size;
};

/**
 * @brief  计算稀疏字库单个字形所需字节数。
 * @param  font 稀疏字库。
 * @return 单个字形所需字节数。
 */
static uint32_t mono_text_sparse_glyph_size(
    const struct mono_sparse_font *font)
{
  uint32_t bytes_per_row;

  bytes_per_row = ((uint32_t)font->width +
                   MONO_TEXT_BITS_PER_BYTE - 1U) /
                  MONO_TEXT_BITS_PER_BYTE;
  return bytes_per_row * font->height;
}

/**
 * @brief  校验稀疏字库的基础元数据。
 * @param  font 稀疏字库。
 * @return true表示有效，false表示无效。
 */
static bool mono_text_sparse_font_is_valid(
    const struct mono_sparse_font *font)
{
  uint32_t glyph_size;

  if ((font == NULL) || (font->width == 0U) ||
      (font->height == 0U) || (font->width > INT16_MAX) ||
      (font->height > INT16_MAX) ||
      ((font->glyph_count > 0U) && (font->glyphs == NULL))) {
    return false;
  }

  glyph_size = mono_text_sparse_glyph_size(font);
  return (font->missing_data == NULL) ||
         (font->missing_data_size >= glyph_size);
}

/**
 * @brief  检查UTF-8续字节。
 * @param  byte 待检查字节。
 * @return true表示是续字节。
 */
static bool mono_text_is_utf8_continuation(uint8_t byte)
{
  return (byte & UINT8_C(0xC0)) == UINT8_C(0x80);
}

/**
 * @brief  解码并校验一个UTF-8编码单元。
 * @param  text 当前字符串位置。
 * @param  token 用于接收编码单元。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_utf8_token_get(const uint8_t *text,
                                    struct mono_text_token *token)
{
  uint8_t first;
  uint8_t second;
  uint8_t length;
  uint8_t index;

  first = text[0];
  if (first < UINT8_C(0x80)) {
    length = 1U;
  } else if ((first >= UINT8_C(0xC2)) &&
             (first <= UINT8_C(0xDF))) {
    length = 2U;
  } else if ((first >= UINT8_C(0xE0)) &&
             (first <= UINT8_C(0xEF))) {
    length = 3U;
  } else if ((first >= UINT8_C(0xF0)) &&
             (first <= UINT8_C(0xF4))) {
    length = 4U;
  } else {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  for (index = 1U; index < length; index++) {
    if ((text[index] == 0U) ||
        !mono_text_is_utf8_continuation(text[index])) {
      return PLATFORM_ERR_INVALID_ARGUMENT;
    }
  }
  second = (length > 1U) ? text[1] : 0U;
  if (((first == UINT8_C(0xE0)) && (second < UINT8_C(0xA0))) ||
      ((first == UINT8_C(0xED)) && (second >= UINT8_C(0xA0))) ||
      ((first == UINT8_C(0xF0)) && (second < UINT8_C(0x90))) ||
      ((first == UINT8_C(0xF4)) && (second > UINT8_C(0x8F)))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  memcpy(token->bytes, text, length);
  token->length = length;
  return PLATFORM_OK;
}

/**
 * @brief  解码并校验一个GB2312编码单元。
 * @param  text 当前字符串位置。
 * @param  token 用于接收编码单元。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_gb2312_token_get(
    const uint8_t *text,
    struct mono_text_token *token)
{
  if (text[0] < UINT8_C(0x80)) {
    token->bytes[0] = text[0];
    token->length = 1U;
    return PLATFORM_OK;
  }
  if ((text[0] < UINT8_C(0xA1)) ||
      (text[0] > UINT8_C(0xF7)) ||
      (text[1] < UINT8_C(0xA1)) ||
      (text[1] > UINT8_C(0xFE))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  token->bytes[0] = text[0];
  token->bytes[1] = text[1];
  token->length = 2U;
  return PLATFORM_OK;
}

/**
 * @brief  按配置编码读取一个编码单元。
 * @param  text 当前字符串位置。
 * @param  encoding 编码宏。
 * @param  token 用于接收编码单元。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_token_get(const uint8_t *text,
                               uint8_t encoding,
                               struct mono_text_token *token)
{
  if ((text == NULL) || (token == NULL) || (text[0] == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (encoding == MONO_TEXT_ENCODING_UTF8) {
    return mono_text_utf8_token_get(text, token);
  }
  if (encoding == MONO_TEXT_ENCODING_GB2312) {
    return mono_text_gb2312_token_get(text, token);
  }
  return PLATFORM_ERR_UNSUPPORTED;
}

/**
 * @brief  在稀疏字库中查找编码单元。
 * @param  font 稀疏字库。
 * @param  token 编码单元。
 * @return 匹配字形；缺字且存在占位字模时返回NULL。
 */
static const struct mono_sparse_glyph *mono_text_sparse_glyph_find(
    const struct mono_sparse_font *font,
    const struct mono_text_token *token)
{
  const struct mono_sparse_glyph *glyph;
  uint16_t index;

  for (index = 0U; index < font->glyph_count; index++) {
    glyph = &font->glyphs[index];
    if ((glyph->code_length == token->length) &&
        (memcmp(glyph->code, token->bytes,
                token->length) == 0)) {
      return glyph;
    }
  }
  return NULL;
}

/**
 * @brief  取得编码单元对应的有效字模。
 * @param  font 稀疏字库。
 * @param  token 编码单元。
 * @param  data 用于接收字模地址。
 * @param  data_size 用于接收字模大小。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_sparse_data_get(
    const struct mono_sparse_font *font,
    const struct mono_text_token *token,
    struct mono_text_glyph_data *glyph_data)
{
  const struct mono_sparse_glyph *glyph;
  uint32_t required_size;

  if (glyph_data == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  glyph = mono_text_sparse_glyph_find(font, token);
  required_size = mono_text_sparse_glyph_size(font);
  if (glyph != NULL) {
    if ((glyph->data == NULL) ||
        (glyph->data_size < required_size)) {
      return PLATFORM_ERR_INVALID_CONFIGURATION;
    }
    glyph_data->data = glyph->data;
    glyph_data->size = glyph->data_size;
    return PLATFORM_OK;
  }
  if (font->missing_data == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  glyph_data->data = font->missing_data;
  glyph_data->size = font->missing_data_size;
  return PLATFORM_OK;
}

/**
 * @brief  获取ASCII兼容尺寸的宽高。
 * @param  ascii_size ASCII尺寸宏。
 * @param  width 用于接收宽度。
 * @param  height 用于接收高度。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_ascii_size_get(uint8_t ascii_size,
                                    uint16_t *width,
                                    uint16_t *height)
{
  if (ascii_size == MONO_TEXT_ASCII_6X8) {
    *width = 6U;
    *height = 8U;
    return PLATFORM_OK;
  }
  if (ascii_size == MONO_TEXT_ASCII_8X16) {
    *width = 8U;
    *height = 16U;
    return PLATFORM_OK;
  }
  return PLATFORM_ERR_UNSUPPORTED;
}

/**
 * @brief  校验完整多字节字符串并测量目标区域。
 * @param  format 多字节文字格式。
 * @param  text 字符串。
 * @param  width 用于接收总宽度。
 * @param  height 用于接收最大高度。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_multibyte_measure(
    const struct mono_multibyte_text_format *format,
    const uint8_t *text,
    uint16_t *width,
    uint16_t *height)
{
  struct mono_text_token token;
  struct mono_text_glyph_data glyph_data;
  uint32_t offset;
  uint32_t total_width;
  uint16_t ascii_width;
  uint16_t ascii_height;
  int result;

  result = mono_text_ascii_size_get(format->ascii_size,
                                    &ascii_width, &ascii_height);
  if (result != PLATFORM_OK) {
    return result;
  }
  total_width = 0U;
  offset = 0U;
  while (text[offset] != 0U) {
    result = mono_text_token_get(&text[offset], format->encoding,
                                 &token);
    if (result != PLATFORM_OK) {
      return result;
    }
    if (token.bytes[0] < UINT8_C(0x80)) {
      if ((token.bytes[0] < MONO_TEXT_ASCII_FIRST) ||
          (token.bytes[0] > MONO_TEXT_ASCII_LAST)) {
        return PLATFORM_ERR_UNSUPPORTED;
      }
      total_width += ascii_width;
    } else {
      result = mono_text_sparse_data_get(format->sparse_font,
                                         &token, &glyph_data);
      if (result != PLATFORM_OK) {
        return result;
      }
      total_width += format->sparse_font->width;
    }
    if (total_width > UINT16_MAX) {
      return PLATFORM_ERR_INVALID_ARGUMENT;
    }
    offset += token.length;
  }
  *width = (uint16_t)total_width;
  *height = (ascii_height > format->sparse_font->height) ?
            ascii_height : format->sparse_font->height;
  return PLATFORM_OK;
}

/**
 * @brief  绘制一个稀疏字形。
 * @param  self 画布对象。
 * @param  x 左上角横坐标。
 * @param  y 左上角纵坐标。
 * @param  font 稀疏字库。
 * @param  data 字模数据。
 * @param  data_size 字模数据大小。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_sparse_glyph_draw(
    struct mono_canvas *self,
    int16_t x,
    int16_t y,
    const struct mono_sparse_font *font,
    const struct mono_text_glyph_data *glyph_data)
{
  struct mono_font glyph_font;
  struct mono_text_style style;

  glyph_font.data = glyph_data->data;
  glyph_font.data_size = glyph_data->size;
  glyph_font.first_char = 0U;
  glyph_font.last_char = 0U;
  glyph_font.width = (uint8_t)font->width;
  glyph_font.height = (uint8_t)font->height;
  style.font = &glyph_font;
  style.width = font->width;
  style.height = font->height;
  return mono_text_draw_char_styled(self, x, y, '\0', &style);
}

/**
 * @brief  绘制UTF-8或GB2312字符串并与ASCII混排。
 * @param  self 已初始化的画布对象。
 * @param  format 编码、坐标、ASCII尺寸和稀疏字库。
 * @param  text 以空字符结尾的字符串。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_draw_multibyte(
    struct mono_canvas *self,
    const struct mono_multibyte_text_format *format,
    const char *text)
{
  struct mono_text_token token;
  struct mono_rect area;
  struct mono_text_glyph_data glyph_data;
  uint32_t offset;
  uint16_t width;
  uint16_t height;
  int32_t cursor_x;
  int result;

  if ((format == NULL) || (text == NULL) ||
      !mono_text_sparse_font_is_valid(format->sparse_font) ||
      (format->sparse_font->width > UINT8_MAX) ||
      (format->sparse_font->height > UINT8_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = mono_text_multibyte_measure(
      format, (const uint8_t *)text, &width, &height);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (width == 0U) {
    return PLATFORM_OK;
  }
  area.x = format->x;
  area.y = format->y;
  area.width = width;
  area.height = height;
  result = mono_canvas_clear_area(self, &area);
  if (result != PLATFORM_OK) {
    return result;
  }

  offset = 0U;
  cursor_x = format->x;
  while (((const uint8_t *)text)[offset] != 0U) {
    result = mono_text_token_get(
        &((const uint8_t *)text)[offset], format->encoding, &token);
    if (result != PLATFORM_OK) {
      return result;
    }
    if (token.bytes[0] < UINT8_C(0x80)) {
      result = mono_text_ascii_draw_char(
          self, (int16_t)cursor_x, format->y,
          (char)token.bytes[0], format->ascii_size);
      cursor_x += (format->ascii_size == MONO_TEXT_ASCII_6X8) ?
                  6 : 8;
    } else {
      result = mono_text_sparse_data_get(
          format->sparse_font, &token, &glyph_data);
      if (result == PLATFORM_OK) {
        result = mono_text_sparse_glyph_draw(
            self, (int16_t)cursor_x, format->y,
            format->sparse_font, &glyph_data);
      }
      cursor_x += format->sparse_font->width;
    }
    if (result != PLATFORM_OK) {
      return result;
    }
    offset += token.length;
  }
  return PLATFORM_OK;
}
