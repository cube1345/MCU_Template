/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_shape.c
 * @brief   单色画布的复杂几何图元绘制实现。
 * @author  weixin
 */
#include "graphics/mono_shape.h"

#include "result.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define MONO_SHAPE_DEGREES_PER_CIRCLE INT32_C(360)
#define MONO_SHAPE_HALF_CIRCLE_DEG INT32_C(180)
#define MONO_SHAPE_RAD_TO_DEG (57.29577951308232f)

struct mono_shape_bounds {
  int32_t min_x;
  int32_t max_x;
  int32_t min_y;
  int32_t max_y;
};

/**
 * @brief  将图元包围盒裁剪到画布范围。
 * @param  self 已初始化的画布对象。
 * @param  bounds 待裁剪的包围盒，函数会原地修改。
 * @return true表示裁剪后仍有可见区域，false表示完全不可见。
 */
static bool mono_shape_bounds_clip(
    const struct mono_canvas *self,
    struct mono_shape_bounds *bounds)
{
  if (bounds->min_x < 0) {
    bounds->min_x = 0;
  }
  if (bounds->min_y < 0) {
    bounds->min_y = 0;
  }
  if (bounds->max_x >= self->width) {
    bounds->max_x = (int32_t)self->width - 1;
  }
  if (bounds->max_y >= self->height) {
    bounds->max_y = (int32_t)self->height - 1;
  }

  return (bounds->min_x <= bounds->max_x) &&
         (bounds->min_y <= bounds->max_y);
}

/**
 * @brief  在画布范围内绘制一个像素，范围外像素直接裁剪。
 * @param  self 已初始化的画布对象。
 * @param  x 横坐标。
 * @param  y 纵坐标。
 * @param  on 像素状态。
 */
static void mono_shape_plot(struct mono_canvas *self,
                            int32_t x,
                            int32_t y,
                            bool on)
{
  if ((x < 0) || (y < 0) ||
      (x >= self->width) || (y >= self->height)) {
    return;
  }

  (void)mono_canvas_draw_pixel(self, (int16_t)x, (int16_t)y, on);
}

/**
 * @brief  绘制允许端点越界的直线。
 * @param  self 已初始化的画布对象。
 * @param  start 起点。
 * @param  end 终点。
 * @param  on 像素状态。
 */
static void mono_shape_draw_line(struct mono_canvas *self,
                                 const struct mono_point *start,
                                 const struct mono_point *end,
                                 bool on)
{
  int32_t x;
  int32_t y;
  int32_t delta_x;
  int32_t delta_y;
  int32_t step_x;
  int32_t step_y;
  int32_t error;
  int32_t error_x2;

  x = start->x;
  y = start->y;
  delta_x = (end->x >= x) ? (end->x - x) : (x - end->x);
  delta_y = (end->y >= y) ? (y - end->y) : (end->y - y);
  step_x = (x < end->x) ? 1 : -1;
  step_y = (y < end->y) ? 1 : -1;
  error = delta_x + delta_y;

  for (;;) {
    mono_shape_plot(self, x, y, on);
    if ((x == end->x) && (y == end->y)) {
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
}

/**
 * @brief  计算有向边与测试点的叉积。
 * @param  start 边起点。
 * @param  end 边终点。
 * @param  x 测试点横坐标。
 * @param  y 测试点纵坐标。
 * @return 有向叉积。
 */
static int64_t mono_shape_edge(const struct mono_point *start,
                               const struct mono_point *end,
                               int32_t x,
                               int32_t y)
{
  return ((int64_t)x - start->x) *
         ((int64_t)end->y - start->y) -
         ((int64_t)y - start->y) *
         ((int64_t)end->x - start->x);
}

/**
 * @brief  判断点是否位于三角形内部或边界上。
 * @param  triangle 三角形。
 * @param  x 测试点横坐标。
 * @param  y 测试点纵坐标。
 * @return true表示位于内部或边界，false表示位于外部。
 */
static bool mono_shape_triangle_contains(
    const struct mono_triangle *triangle,
    int32_t x,
    int32_t y)
{
  int64_t edge_0;
  int64_t edge_1;
  int64_t edge_2;
  bool has_negative;
  bool has_positive;

  edge_0 = mono_shape_edge(&triangle->vertices[0],
                           &triangle->vertices[1], x, y);
  edge_1 = mono_shape_edge(&triangle->vertices[1],
                           &triangle->vertices[2], x, y);
  edge_2 = mono_shape_edge(&triangle->vertices[2],
                           &triangle->vertices[0], x, y);
  has_negative = (edge_0 < 0) || (edge_1 < 0) || (edge_2 < 0);
  has_positive = (edge_0 > 0) || (edge_1 > 0) || (edge_2 > 0);
  return !(has_negative && has_positive);
}

/**
 * @brief  计算三角形包围盒。
 * @param  triangle 三角形。
 * @return 未裁剪的包围盒。
 */
static struct mono_shape_bounds mono_shape_triangle_bounds(
    const struct mono_triangle *triangle)
{
  struct mono_shape_bounds bounds;
  uint16_t index;

  bounds.min_x = triangle->vertices[0].x;
  bounds.max_x = triangle->vertices[0].x;
  bounds.min_y = triangle->vertices[0].y;
  bounds.max_y = triangle->vertices[0].y;
  for (index = 1U; index < 3U; index++) {
    if (triangle->vertices[index].x < bounds.min_x) {
      bounds.min_x = triangle->vertices[index].x;
    }
    if (triangle->vertices[index].x > bounds.max_x) {
      bounds.max_x = triangle->vertices[index].x;
    }
    if (triangle->vertices[index].y < bounds.min_y) {
      bounds.min_y = triangle->vertices[index].y;
    }
    if (triangle->vertices[index].y > bounds.max_y) {
      bounds.max_y = triangle->vertices[index].y;
    }
  }
  return bounds;
}

/**
 * @brief  绘制空心或填充三角形。
 * @param  self 已初始化的画布对象。
 * @param  triangle 三角形顶点。
 * @param  filled 是否填充。
 * @param  on 像素状态。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_shape_draw_triangle(struct mono_canvas *self,
                             const struct mono_triangle *triangle,
                             bool filled,
                             bool on)
{
  struct mono_shape_bounds bounds;
  int32_t x;
  int32_t y;
  int64_t area;

  if (!mono_canvas_is_initialized(self) || (triangle == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  area = mono_shape_edge(&triangle->vertices[0],
                         &triangle->vertices[1],
                         triangle->vertices[2].x,
                         triangle->vertices[2].y);
  if (filled && (area == 0)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  if (!filled) {
    mono_shape_draw_line(self, &triangle->vertices[0],
                         &triangle->vertices[1], on);
    mono_shape_draw_line(self, &triangle->vertices[1],
                         &triangle->vertices[2], on);
    mono_shape_draw_line(self, &triangle->vertices[2],
                         &triangle->vertices[0], on);
    return PLATFORM_OK;
  }

  bounds = mono_shape_triangle_bounds(triangle);
  if (!mono_shape_bounds_clip(self, &bounds)) {
    return PLATFORM_OK;
  }
  for (y = bounds.min_y; y <= bounds.max_y; y++) {
    for (x = bounds.min_x; x <= bounds.max_x; x++) {
      if (mono_shape_triangle_contains(triangle, x, y)) {
        mono_shape_plot(self, x, y, on);
      }
    }
  }
  return PLATFORM_OK;
}

/**
 * @brief  判断点是否位于圆内。
 * @param  dx 点相对圆心的横向偏移。
 * @param  dy 点相对圆心的纵向偏移。
 * @param  radius 圆半径。
 * @return true表示位于圆内或圆周上。
 */
static bool mono_shape_circle_contains(int32_t dx,
                                       int32_t dy,
                                       uint16_t radius)
{
  int64_t distance_squared;
  int64_t radius_squared;

  distance_squared = (int64_t)dx * dx + (int64_t)dy * dy;
  radius_squared = (int64_t)radius * radius;
  return distance_squared <= radius_squared;
}

/**
 * @brief  判断点是否位于一像素宽的圆周上。
 * @param  dx 点相对圆心的横向偏移。
 * @param  dy 点相对圆心的纵向偏移。
 * @param  radius 圆半径。
 * @return true表示位于圆周上。
 */
static bool mono_shape_circle_boundary(int32_t dx,
                                       int32_t dy,
                                       uint16_t radius)
{
  if (!mono_shape_circle_contains(dx, dy, radius)) {
    return false;
  }

  return !mono_shape_circle_contains(dx - 1, dy, radius) ||
         !mono_shape_circle_contains(dx + 1, dy, radius) ||
         !mono_shape_circle_contains(dx, dy - 1, radius) ||
         !mono_shape_circle_contains(dx, dy + 1, radius);
}

/**
 * @brief  绘制空心或填充圆。
 * @param  self 已初始化的画布对象。
 * @param  circle 圆心和半径。
 * @param  filled 是否填充。
 * @param  on 像素状态。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_shape_draw_circle(struct mono_canvas *self,
                           const struct mono_circle *circle,
                           bool filled,
                           bool on)
{
  struct mono_shape_bounds bounds;
  int32_t x;
  int32_t y;
  int32_t dx;
  int32_t dy;
  bool selected;

  if (!mono_canvas_is_initialized(self) || (circle == NULL) ||
      (circle->radius == 0U) || (circle->radius > INT16_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  bounds.min_x = (int32_t)circle->center.x - circle->radius;
  bounds.max_x = (int32_t)circle->center.x + circle->radius;
  bounds.min_y = (int32_t)circle->center.y - circle->radius;
  bounds.max_y = (int32_t)circle->center.y + circle->radius;
  if (!mono_shape_bounds_clip(self, &bounds)) {
    return PLATFORM_OK;
  }

  for (y = bounds.min_y; y <= bounds.max_y; y++) {
    for (x = bounds.min_x; x <= bounds.max_x; x++) {
      dx = x - circle->center.x;
      dy = y - circle->center.y;
      selected = filled ? mono_shape_circle_contains(
                              dx, dy, circle->radius) :
                          mono_shape_circle_boundary(
                              dx, dy, circle->radius);
      if (selected) {
        mono_shape_plot(self, x, y, on);
      }
    }
  }
  return PLATFORM_OK;
}

/**
 * @brief  判断点是否位于椭圆内。
 * @param  dx 点相对中心的横向偏移。
 * @param  dy 点相对中心的纵向偏移。
 * @param  radius_x 横向半轴。
 * @param  radius_y 纵向半轴。
 * @return true表示位于椭圆内或边界上。
 */
static bool mono_shape_ellipse_contains(int32_t dx,
                                        int32_t dy,
                                        uint16_t radius_x,
                                        uint16_t radius_y)
{
  int64_t radius_x_squared;
  int64_t radius_y_squared;
  int64_t value;
  int64_t limit;

  radius_x_squared = (int64_t)radius_x * radius_x;
  radius_y_squared = (int64_t)radius_y * radius_y;
  value = (int64_t)dx * dx * radius_y_squared +
          (int64_t)dy * dy * radius_x_squared;
  limit = radius_x_squared * radius_y_squared;
  return value <= limit;
}

/**
 * @brief  判断点是否位于一像素宽的椭圆边界。
 * @param  ellipse 椭圆对象。
 * @param  dx 点相对中心的横向偏移。
 * @param  dy 点相对中心的纵向偏移。
 * @return true表示位于椭圆边界。
 */
static bool mono_shape_ellipse_boundary(
    const struct mono_ellipse *ellipse,
    int32_t dx,
    int32_t dy)
{
  uint16_t radius_x;
  uint16_t radius_y;

  radius_x = ellipse->radius_x;
  radius_y = ellipse->radius_y;
  if (!mono_shape_ellipse_contains(dx, dy, radius_x, radius_y)) {
    return false;
  }

  return !mono_shape_ellipse_contains(dx - 1, dy,
                                      radius_x, radius_y) ||
         !mono_shape_ellipse_contains(dx + 1, dy,
                                      radius_x, radius_y) ||
         !mono_shape_ellipse_contains(dx, dy - 1,
                                      radius_x, radius_y) ||
         !mono_shape_ellipse_contains(dx, dy + 1,
                                      radius_x, radius_y);
}

/**
 * @brief  绘制空心或填充椭圆。
 * @param  self 已初始化的画布对象。
 * @param  ellipse 中心和横纵半轴。
 * @param  filled 是否填充。
 * @param  on 像素状态。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_shape_draw_ellipse(struct mono_canvas *self,
                            const struct mono_ellipse *ellipse,
                            bool filled,
                            bool on)
{
  struct mono_shape_bounds bounds;
  int32_t x;
  int32_t y;
  int32_t dx;
  int32_t dy;
  bool selected;

  if (!mono_canvas_is_initialized(self) || (ellipse == NULL) ||
      (ellipse->radius_x == 0U) || (ellipse->radius_y == 0U) ||
      (ellipse->radius_x > INT16_MAX) ||
      (ellipse->radius_y > INT16_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  bounds.min_x = (int32_t)ellipse->center.x - ellipse->radius_x;
  bounds.max_x = (int32_t)ellipse->center.x + ellipse->radius_x;
  bounds.min_y = (int32_t)ellipse->center.y - ellipse->radius_y;
  bounds.max_y = (int32_t)ellipse->center.y + ellipse->radius_y;
  if (!mono_shape_bounds_clip(self, &bounds)) {
    return PLATFORM_OK;
  }

  for (y = bounds.min_y; y <= bounds.max_y; y++) {
    for (x = bounds.min_x; x <= bounds.max_x; x++) {
      dx = x - ellipse->center.x;
      dy = y - ellipse->center.y;
      selected = filled ? mono_shape_ellipse_contains(
                              dx, dy, ellipse->radius_x,
                              ellipse->radius_y) :
                          mono_shape_ellipse_boundary(
                              ellipse, dx, dy);
      if (selected) {
        mono_shape_plot(self, x, y, on);
      }
    }
  }
  return PLATFORM_OK;
}

/**
 * @brief  将任意角度归一化到负180至正180度。
 * @param  angle_deg 原始角度。
 * @return 归一化后的角度。
 */
static int32_t mono_shape_angle_normalize(int32_t angle_deg)
{
  int32_t normalized;

  normalized = angle_deg % MONO_SHAPE_DEGREES_PER_CIRCLE;
  if (normalized > MONO_SHAPE_HALF_CIRCLE_DEG) {
    normalized -= MONO_SHAPE_DEGREES_PER_CIRCLE;
  } else if (normalized < -MONO_SHAPE_HALF_CIRCLE_DEG) {
    normalized += MONO_SHAPE_DEGREES_PER_CIRCLE;
  }
  return normalized;
}

/**
 * @brief  判断相对坐标是否位于圆弧角度范围。
 * @param  dx 点相对圆心的横向偏移。
 * @param  dy 点相对圆心的纵向偏移。
 * @param  arc 圆弧对象。
 * @return true表示角度命中或点位于圆心。
 */
static bool mono_shape_arc_angle_contains(
    int32_t dx,
    int32_t dy,
    const struct mono_arc *arc)
{
  int32_t start_angle;
  int32_t end_angle;
  int32_t point_angle;

  if ((dx == 0) && (dy == 0)) {
    return true;
  }

  start_angle = mono_shape_angle_normalize(arc->start_angle_deg);
  end_angle = mono_shape_angle_normalize(arc->end_angle_deg);
  if (start_angle == end_angle) {
    return true;
  }
  point_angle = (int32_t)(atan2f((float)dy, (float)dx) *
                          MONO_SHAPE_RAD_TO_DEG);
  if (start_angle < end_angle) {
    return (point_angle >= start_angle) &&
           (point_angle <= end_angle);
  }
  return (point_angle >= start_angle) ||
         (point_angle <= end_angle);
}

/**
 * @brief  绘制空心圆弧或填充扇形。
 * @param  self 已初始化的画布对象。
 * @param  arc 圆心、半径和起止角度。
 * @param  filled 是否填充扇形。
 * @param  on 像素状态。
 * @return PLATFORM_OK表示成功，其余PLATFORM_*表示失败。
 */
int mono_shape_draw_arc(struct mono_canvas *self,
                        const struct mono_arc *arc,
                        bool filled,
                        bool on)
{
  struct mono_shape_bounds bounds;
  int32_t x;
  int32_t y;
  int32_t dx;
  int32_t dy;
  bool radial_match;
  bool selected;

  if (!mono_canvas_is_initialized(self) || (arc == NULL) ||
      (arc->radius == 0U) || (arc->radius > INT16_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  bounds.min_x = (int32_t)arc->center.x - arc->radius;
  bounds.max_x = (int32_t)arc->center.x + arc->radius;
  bounds.min_y = (int32_t)arc->center.y - arc->radius;
  bounds.max_y = (int32_t)arc->center.y + arc->radius;
  if (!mono_shape_bounds_clip(self, &bounds)) {
    return PLATFORM_OK;
  }

  for (y = bounds.min_y; y <= bounds.max_y; y++) {
    for (x = bounds.min_x; x <= bounds.max_x; x++) {
      dx = x - arc->center.x;
      dy = y - arc->center.y;
      radial_match = filled ? mono_shape_circle_contains(
                                  dx, dy, arc->radius) :
                              mono_shape_circle_boundary(
                                  dx, dy, arc->radius);
      selected = radial_match &&
                 mono_shape_arc_angle_contains(dx, dy, arc);
      if (selected) {
        mono_shape_plot(self, x, y, on);
      }
    }
  }
  return PLATFORM_OK;
}
