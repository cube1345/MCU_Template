/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_font.h
 * @brief   单色点阵字体的只读数据描述。
 * @author  weixin
 *
 * 字模按字符连续存储。每个字符内部按行排列，每行从左到右，
 * 每个字节的最低位对应最左侧像素。
 */
#ifndef MONO_FONT_H
#define MONO_FONT_H

#include <stdint.h>

struct mono_font {
  /** 字模数据，只读且生命周期必须长于所有绘制调用。 */
  const uint8_t *data;
  /** 字模数据总字节数，用于运行时边界校验。 */
  uint32_t data_size;
  /** 字体支持的第一个字符编码。 */
  uint8_t first_char;
  /** 字体支持的最后一个字符编码。 */
  uint8_t last_char;
  /** 单个字符的固定像素宽度。 */
  uint8_t width;
  /** 单个字符的固定像素高度。 */
  uint8_t height;
};

#endif /* MONO_FONT_H */
