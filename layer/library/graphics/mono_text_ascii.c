/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_ascii.c
 * @brief   兼容6x8和8x16尺寸的ASCII文字绘制实现。
 * @author  weixin
 */
#include "graphics/mono_text_ascii.h"

#include "graphics/font/mono_font_ascii.h"
#include "graphics/mono_text.h"
#include "result.h"

#include <stddef.h>

/**
 * @brief  根据兼容尺寸构造一次性文字样式。
 * @param  text_size 兼容字体尺寸。
 * @param  style 用于接收样式。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
static int mono_text_ascii_style_get(
    uint8_t text_size,
    struct mono_text_style *style)
{
  if (style == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  style->font = mono_font_ascii_8x8_get();
  if (text_size == MONO_TEXT_ASCII_6X8) {
    style->width = 6U;
    style->height = 8U;
    return PLATFORM_OK;
  }
  if (text_size == MONO_TEXT_ASCII_8X16) {
    style->width = 8U;
    style->height = 16U;
    return PLATFORM_OK;
  }
  return PLATFORM_ERR_UNSUPPORTED;
}

/**
 * @brief  使用内置ASCII字体绘制单个字符。
 * @param  self 已初始化的画布对象。
 * @param  x 字符左上角横坐标。
 * @param  y 字符左上角纵坐标。
 * @param  character 可打印ASCII字符。
 * @param  text_size 兼容字体尺寸。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_ascii_draw_char(struct mono_canvas *self,
                              int16_t x,
                              int16_t y,
                              char character,
                              uint8_t text_size)
{
  struct mono_text_style style;
  int result;

  result = mono_text_ascii_style_get(text_size, &style);
  if (result != PLATFORM_OK) {
    return result;
  }
  return mono_text_draw_char_styled(self, x, y,
                                    character, &style);
}

/**
 * @brief  使用内置ASCII字体绘制字符串。
 * @param  self 已初始化的画布对象。
 * @param  x 字符串左上角横坐标。
 * @param  y 字符串左上角纵坐标。
 * @param  text 可打印ASCII字符串。
 * @param  text_size 兼容字体尺寸。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_text_ascii_draw_string(struct mono_canvas *self,
                                int16_t x,
                                int16_t y,
                                const char *text,
                                uint8_t text_size)
{
  struct mono_text_style style;
  int result;

  result = mono_text_ascii_style_get(text_size, &style);
  if (result != PLATFORM_OK) {
    return result;
  }
  return mono_text_draw_string_styled(self, x, y, text, &style);
}
