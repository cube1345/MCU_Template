/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_canvas.c
 * @brief   单色垂直分页显存的像素和基础图形实现。
 * @author  weixin
 */
#include "graphics/mono_canvas.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#define MONO_CANVAS_BITS_PER_PAGE UINT32_C(8)

/**
 * @brief  计算画布需要的显存大小。
 * @param  width 画布宽度。
 * @param  height 画布高度。
 * @return 所需字节数。
 */
static uint32_t mono_canvas_required_size(uint16_t width,
                                          uint16_t height)
{
  uint32_t pages;

  pages = ((uint32_t)height + MONO_CANVAS_BITS_PER_PAGE - 1U) /
          MONO_CANVAS_BITS_PER_PAGE;
  return (uint32_t)width * pages;
}

/**
 * @brief  检查画布对象是否具有有效显存。
 * @param  self 画布对象。
 * @return true 表示可用；false 表示无效。
 */
bool mono_canvas_is_initialized(const struct mono_canvas *self)
{
  uint32_t required_size;

  if ((self == NULL) || (self->buffer == NULL) ||
      (self->width == 0U) || (self->height == 0U)) {
    return false;
  }

  required_size = mono_canvas_required_size(self->width,
                                             self->height);
  return self->buffer_size >= required_size;
}

/**
 * @brief  判断坐标是否位于画布内。
 * @param  self 画布对象。
 * @param  x 横坐标。
 * @param  y 纵坐标。
 * @return true 表示有效；false 表示越界。
 */
static bool mono_canvas_point_is_valid(
    const struct mono_canvas *self,
    int16_t x,
    int16_t y)
{
  return mono_canvas_is_initialized(self) && (x >= 0) && (y >= 0) &&
         ((uint16_t)x < self->width) &&
         ((uint16_t)y < self->height);
}

/**
 * @brief  判断矩形是否完整位于画布内。
 * @param  self 画布对象。
 * @param  rect 矩形参数。
 * @return true 表示有效；false 表示无效或越界。
 */
static bool mono_canvas_rect_is_valid(
    const struct mono_canvas *self,
    const struct mono_rect *rect)
{
  uint32_t end_x;
  uint32_t end_y;

  if (!mono_canvas_is_initialized(self) || (rect == NULL) ||
      (rect->x < 0) || (rect->y < 0) ||
      (rect->width == 0U) || (rect->height == 0U)) {
    return false;
  }

  end_x = (uint32_t)(uint16_t)rect->x + rect->width;
  end_y = (uint32_t)(uint16_t)rect->y + rect->height;
  return (end_x <= self->width) && (end_y <= self->height);
}

/**
 * @brief  初始化单色画布。
 * @param  self 调用者提供的画布对象。
 * @param  buffer 垂直分页显存。
 * @param  buffer_size 显存大小。
 * @param  width 画布宽度。
 * @param  height 画布高度。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_init(struct mono_canvas *self,
                     uint8_t *buffer,
                     uint32_t buffer_size,
                     uint16_t width,
                     uint16_t height)
{
  uint32_t required_size;

  if ((self == NULL) || (buffer == NULL) ||
      (width == 0U) || (height == 0U) ||
      (width > INT16_MAX) || (height > INT16_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  required_size = mono_canvas_required_size(width, height);
  if ((required_size == 0U) || (buffer_size < required_size)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  self->buffer = buffer;
  self->buffer_size = buffer_size;
  self->width = width;
  self->height = height;
  return PLATFORM_OK;
}

/**
 * @brief  将画布全部设置为亮或灭。
 * @param  self 已初始化的画布对象。
 * @param  on true 表示全部置位，false 表示全部清零。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_clear(struct mono_canvas *self, bool on)
{
  uint32_t required_size;

  if (!mono_canvas_is_initialized(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  required_size = mono_canvas_required_size(self->width,
                                             self->height);
  memset(self->buffer, on ? 0xFF : 0x00, required_size);
  return PLATFORM_OK;
}

/**
 * @brief  清除指定矩形区域。
 * @param  self 已初始化的画布对象。
 * @param  area 要清除的区域。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_canvas_clear_area(struct mono_canvas *self,
                           const struct mono_rect *area)
{
  return mono_canvas_draw_rect(self, area, true, false);
}

/**
 * @brief  翻转整个画布的所有像素。
 * @param  self 已初始化的画布对象。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_canvas_invert(struct mono_canvas *self)
{
  uint32_t index;
  uint32_t required_size;

  if (!mono_canvas_is_initialized(self)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  required_size = mono_canvas_required_size(self->width,
                                             self->height);
  for (index = 0U; index < required_size; index++) {
    self->buffer[index] = (uint8_t)~self->buffer[index];
  }
  return PLATFORM_OK;
}

/**
 * @brief  翻转指定矩形区域内的所有像素。
 * @param  self 已初始化的画布对象。
 * @param  area 要翻转的区域。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_canvas_invert_area(struct mono_canvas *self,
                            const struct mono_rect *area)
{
  uint16_t offset_x;
  uint16_t offset_y;
  bool on;

  if (!mono_canvas_rect_is_valid(self, area)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  for (offset_y = 0U; offset_y < area->height; offset_y++) {
    for (offset_x = 0U; offset_x < area->width; offset_x++) {
      (void)mono_canvas_get_pixel(
          self, (int16_t)(area->x + (int16_t)offset_x),
          (int16_t)(area->y + (int16_t)offset_y), &on);
      (void)mono_canvas_draw_pixel(
          self, (int16_t)(area->x + (int16_t)offset_x),
          (int16_t)(area->y + (int16_t)offset_y), !on);
    }
  }
  return PLATFORM_OK;
}

/**
 * @brief  设置一个像素。
 * @param  self 已初始化的画布对象。
 * @param  x 像素横坐标。
 * @param  y 像素纵坐标。
 * @param  on true 表示点亮，false 表示熄灭。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_pixel(struct mono_canvas *self,
                           int16_t x,
                           int16_t y,
                           bool on)
{
  uint32_t index;
  uint8_t mask;

  if (!mono_canvas_point_is_valid(self, x, y)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  index = ((uint32_t)(uint16_t)y / MONO_CANVAS_BITS_PER_PAGE) *
          self->width + (uint16_t)x;
  mask = (uint8_t)(UINT8_C(1) << ((uint16_t)y & UINT16_C(7)));

  if (on) {
    self->buffer[index] |= mask;
  } else {
    self->buffer[index] &= (uint8_t)~mask;
  }
  return PLATFORM_OK;
}

/**
 * @brief  读取一个像素。
 * @param  self 已初始化的画布对象。
 * @param  x 像素横坐标。
 * @param  y 像素纵坐标。
 * @param  on 用于接收像素状态的变量。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_get_pixel(const struct mono_canvas *self,
                          int16_t x,
                          int16_t y,
                          bool *on)
{
  uint32_t index;
  uint8_t mask;

  if ((on == NULL) || !mono_canvas_point_is_valid(self, x, y)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  index = ((uint32_t)(uint16_t)y / MONO_CANVAS_BITS_PER_PAGE) *
          self->width + (uint16_t)x;
  mask = (uint8_t)(UINT8_C(1) << ((uint16_t)y & UINT16_C(7)));
  *on = (self->buffer[index] & mask) != 0U;
  return PLATFORM_OK;
}

/**
 * @brief  绘制一条直线。
 * @param  self 已初始化的画布对象。
 * @param  line 起点和终点。
 * @param  on true 表示点亮，false 表示擦除。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_line(struct mono_canvas *self,
                          const struct mono_line *line,
                          bool on)
{
  int32_t x;
  int32_t y;
  int32_t end_x;
  int32_t end_y;
  int32_t delta_x;
  int32_t delta_y;
  int32_t step_x;
  int32_t step_y;
  int32_t error;
  int32_t error_x2;

  if ((line == NULL) ||
      !mono_canvas_point_is_valid(self, line->start.x,
                                  line->start.y) ||
      !mono_canvas_point_is_valid(self, line->end.x,
                                  line->end.y)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  x = line->start.x;
  y = line->start.y;
  end_x = line->end.x;
  end_y = line->end.y;
  delta_x = (end_x >= x) ? (end_x - x) : (x - end_x);
  delta_y = (end_y >= y) ? (y - end_y) : (end_y - y);
  step_x = (x < end_x) ? 1 : -1;
  step_y = (y < end_y) ? 1 : -1;
  error = delta_x + delta_y;

  for (;;) {
    (void)mono_canvas_draw_pixel(self, (int16_t)x,
                                 (int16_t)y, on);
    if ((x == end_x) && (y == end_y)) {
      break;
    }
    error_x2 = error * 2;
    if (error_x2 >= delta_y) {
      error += delta_y;
      x += step_x;
    }
    if (error_x2 <= delta_x) {
      error += delta_x;
      y += step_y;
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  绘制空心或实心矩形。
 * @param  self 已初始化的画布对象。
 * @param  rect 矩形位置和尺寸。
 * @param  filled true 表示填充，false 表示只画边框。
 * @param  on true 表示点亮，false 表示擦除。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_rect(struct mono_canvas *self,
                          const struct mono_rect *rect,
                          bool filled,
                          bool on)
{
  uint16_t offset_x;
  uint16_t offset_y;
  bool border;

  if (!mono_canvas_rect_is_valid(self, rect)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  for (offset_y = 0U; offset_y < rect->height; offset_y++) {
    for (offset_x = 0U; offset_x < rect->width; offset_x++) {
      border = (offset_x == 0U) ||
               (offset_x == (rect->width - 1U)) ||
               (offset_y == 0U) ||
               (offset_y == (rect->height - 1U));
      if (filled || border) {
        (void)mono_canvas_draw_pixel(
            self,
            (int16_t)(rect->x + (int16_t)offset_x),
            (int16_t)(rect->y + (int16_t)offset_y),
            on);
      }
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  绘制垂直分页格式的单色位图。
 * @param  self 已初始化的画布对象。
 * @param  x 位图左上角横坐标。
 * @param  y 位图左上角纵坐标。
 * @param  bitmap 位图数据及尺寸。
 * @param  transparent true 表示忽略零位，false 表示完整覆盖。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mono_canvas_draw_bitmap(struct mono_canvas *self,
                            int16_t x,
                            int16_t y,
                            const struct mono_bitmap *bitmap,
                            bool transparent)
{
  struct mono_rect area;
  uint16_t source_x;
  uint16_t source_y;
  uint32_t source_index;
  uint32_t required_size;
  uint8_t source_mask;
  bool on;

  if ((bitmap == NULL) || (bitmap->data == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  required_size = mono_canvas_required_size(bitmap->width,
                                             bitmap->height);
  if ((required_size == 0U) ||
      (bitmap->data_size < required_size)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  area.x = x;
  area.y = y;
  area.width = bitmap->width;
  area.height = bitmap->height;
  if (!mono_canvas_rect_is_valid(self, &area)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  for (source_y = 0U; source_y < bitmap->height; source_y++) {
    for (source_x = 0U; source_x < bitmap->width; source_x++) {
      source_index = ((uint32_t)source_y /
                      MONO_CANVAS_BITS_PER_PAGE) *
                     bitmap->width + source_x;
      source_mask = (uint8_t)(UINT8_C(1) <<
                              (source_y & UINT16_C(7)));
      on = (bitmap->data[source_index] & source_mask) != 0U;
      if (on || !transparent) {
        (void)mono_canvas_draw_pixel(
            self,
            (int16_t)(x + (int16_t)source_x),
            (int16_t)(y + (int16_t)source_y),
            on);
      }
    }
  }

  return PLATFORM_OK;
}
