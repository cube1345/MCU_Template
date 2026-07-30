/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_text_multibyte.h
 * @brief   UTF-8或GB2312稀疏字库与ASCII混排接口。
 * @author  weixin
 */
#ifndef MONO_TEXT_MULTIBYTE_H
#define MONO_TEXT_MULTIBYTE_H

#include "graphics/mono_canvas.h"

#include <stdint.h>

#define MONO_TEXT_ENCODING_UTF8 UINT8_C(1)
#define MONO_TEXT_ENCODING_GB2312 UINT8_C(2)
#define MONO_TEXT_MULTIBYTE_CODE_SIZE UINT8_C(4)

struct mono_sparse_glyph {
  /** UTF-8或GB2312原始编码，不包含结尾空字符。 */
  uint8_t code[MONO_TEXT_MULTIBYTE_CODE_SIZE];
  /** code中的有效字节数。 */
  uint8_t code_length;
  /** 行优先字模数据，行内最低位对应最左像素。 */
  const uint8_t *data;
  /** 字模数据字节数。 */
  uint32_t data_size;
};

struct mono_sparse_font {
  /** 稀疏字形表，生命周期必须覆盖所有绘制调用。 */
  const struct mono_sparse_glyph *glyphs;
  /** 字形表元素数量。 */
  uint16_t glyph_count;
  /** 所有字形的固定宽度。 */
  uint16_t width;
  /** 所有字形的固定高度。 */
  uint16_t height;
  /** 可选缺字字模；为NULL时遇到缺字返回不支持。 */
  const uint8_t *missing_data;
  /** 缺字字模数据字节数。 */
  uint32_t missing_data_size;
};

struct mono_multibyte_text_format {
  int16_t x;
  int16_t y;
  uint8_t ascii_size;
  uint8_t encoding;
  const struct mono_sparse_font *sparse_font;
};

/**
 * @brief  绘制UTF-8或GB2312字符串，并与可打印ASCII混排。
 * @param  self 已初始化的画布对象。
 * @param  format 坐标、ASCII尺寸、编码方式和稀疏字库。
 * @param  text 以空字符结尾且符合format编码的字符串。
 * @return PLATFORM_OK表示成功；编码、字形或区域无效时返回对应错误。
 * @note   字符串会先完整校验，不自动换行；字库和字符串均由调用者持有，
 *         函数不分配内存、不刷新设备且不保证线程安全。
 */
int mono_text_draw_multibyte(
    struct mono_canvas *self,
    const struct mono_multibyte_text_format *format,
    const char *text);

#endif /* MONO_TEXT_MULTIBYTE_H */
