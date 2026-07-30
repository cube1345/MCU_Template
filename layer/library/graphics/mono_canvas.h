/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_canvas.h
 * @brief   单色垂直分页显存的基础绘图接口。
 * @author  weixin
 *
 * 本模块只修改调用者提供的内存，不访问显示设备、总线、HAL或RTOS。
 */
#ifndef MONO_CANVAS_H
#define MONO_CANVAS_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

struct mono_canvas {
  uint8_t *buffer;
  uint32_t buffer_size;
  uint16_t width;
  uint16_t height;
};

struct mono_point {
  int16_t x;
  int16_t y;
};

struct mono_line {
  struct mono_point start;
  struct mono_point end;
};

struct mono_rect {
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
};

struct mono_bitmap {
  const uint8_t *data;
  uint32_t data_size;
  uint16_t width;
  uint16_t height;
};

/**
 * @brief  初始化单色画布。
 * @param  self 调用者提供的画布对象。
 * @param  buffer 垂直分页显存，生命周期必须长于 self。
 * @param  buffer_size 显存大小，单位为字节。
 * @param  width 画布宽度，单位为像素。
 * @param  height 画布高度，单位为像素。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   self必须由调用者清零；画布不分配显存，也不保证线程安全。
 */
int mono_canvas_init(struct mono_canvas *self,
                     uint8_t *buffer,
                     uint32_t buffer_size,
                     uint16_t width,
                     uint16_t height);

/**
 * @brief  查询单色画布是否已经正确初始化。
 * @param  self 待查询的画布对象。
 * @return true表示画布和显存有效，false表示对象无效或显存不足。
 * @note   本函数只读取对象状态，不修改显存，可在调用者已保证对象生命
 *         周期有效的任意上下文调用。
 */
bool mono_canvas_is_initialized(const struct mono_canvas *self);

/**
 * @brief  将画布全部设置为亮或灭。
 * @param  self 已初始化的画布对象。
 * @param  on true 表示全部置位，false 表示全部清零。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_clear(struct mono_canvas *self, bool on);

/**
 * @brief  清除指定矩形区域。
 * @param  self 已初始化的画布对象。
 * @param  area 要清除的区域，必须完整位于画布内。
 * @return PLATFORM_OK表示成功；参数或画布无效时返回对应错误。
 * @note   本函数只修改显存，不刷新显示设备，也不保证线程安全。
 */
int mono_canvas_clear_area(struct mono_canvas *self,
                           const struct mono_rect *area);

/**
 * @brief  翻转整个画布的所有像素。
 * @param  self 已初始化的画布对象。
 * @return PLATFORM_OK表示成功；画布无效时返回
 *         PLATFORM_ERR_INVALID_STATE。
 * @note   本函数只修改显存，不刷新显示设备，也不保证线程安全。
 */
int mono_canvas_invert(struct mono_canvas *self);

/**
 * @brief  翻转指定矩形区域内的所有像素。
 * @param  self 已初始化的画布对象。
 * @param  area 要翻转的区域，必须完整位于画布内。
 * @return PLATFORM_OK表示成功；参数或画布无效时返回对应错误。
 * @note   本函数只修改显存，不刷新显示设备，也不保证线程安全。
 */
int mono_canvas_invert_area(struct mono_canvas *self,
                            const struct mono_rect *area);

/**
 * @brief  设置一个像素。
 * @param  self 已初始化的画布对象。
 * @param  x 像素横坐标。
 * @param  y 像素纵坐标。
 * @param  on true 表示点亮，false 表示熄灭。
 * @return PLATFORM_OK 表示成功；越界时返回参数错误。
 */
int mono_canvas_draw_pixel(struct mono_canvas *self,
                           int16_t x,
                           int16_t y,
                           bool on);

/**
 * @brief  读取一个像素。
 * @param  self 已初始化的画布对象。
 * @param  x 像素横坐标。
 * @param  y 像素纵坐标。
 * @param  on 用于接收像素状态的变量。
 * @return PLATFORM_OK 表示成功；越界或参数无效时返回错误。
 */
int mono_canvas_get_pixel(const struct mono_canvas *self,
                          int16_t x,
                          int16_t y,
                          bool *on);

/**
 * @brief  绘制一条直线。
 * @param  self 已初始化的画布对象。
 * @param  line 起点和终点，必须全部位于画布内。
 * @param  on true 表示点亮，false 表示擦除。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_line(struct mono_canvas *self,
                          const struct mono_line *line,
                          bool on);

/**
 * @brief  绘制空心或实心矩形。
 * @param  self 已初始化的画布对象。
 * @param  rect 矩形位置和尺寸，必须完全位于画布内。
 * @param  filled true 表示填充矩形，false 表示只画边框。
 * @param  on true 表示点亮，false 表示擦除。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_rect(struct mono_canvas *self,
                          const struct mono_rect *rect,
                          bool filled,
                          bool on);

/**
 * @brief  绘制垂直分页格式的单色位图。
 * @param  self 已初始化的画布对象。
 * @param  x 位图左上角横坐标。
 * @param  y 位图左上角纵坐标。
 * @param  bitmap 位图数据及尺寸，数据按每8行一页排列。
 * @param  transparent true 表示忽略零位，false 表示完整覆盖。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_bitmap(struct mono_canvas *self,
                            int16_t x,
                            int16_t y,
                            const struct mono_bitmap *bitmap,
                            bool transparent);

#endif /* MONO_CANVAS_H */
