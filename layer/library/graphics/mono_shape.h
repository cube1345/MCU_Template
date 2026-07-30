/* SPDX-License-Identifier: MIT */
/**
 * @file    mono_shape.h
 * @brief   单色画布的三角形、圆、椭圆和圆弧绘制接口。
 * @author  weixin
 *
 * 图元允许部分位于画布外，超出画布的像素会被安全裁剪。
 */
#ifndef MONO_SHAPE_H
#define MONO_SHAPE_H

#include "graphics/mono_canvas.h"

#include <stdbool.h>
#include <stdint.h>

struct mono_triangle {
  struct mono_point vertices[3];
};

struct mono_circle {
  struct mono_point center;
  uint16_t radius;
};

struct mono_ellipse {
  struct mono_point center;
  uint16_t radius_x;
  uint16_t radius_y;
};

struct mono_arc {
  struct mono_point center;
  uint16_t radius;
  int16_t start_angle_deg;
  int16_t end_angle_deg;
};

/**
 * @brief  绘制空心或填充三角形。
 * @param  self 已初始化的画布对象。
 * @param  triangle 三角形的三个顶点。
 * @param  filled true表示填充，false表示只绘制三条边。
 * @param  on true表示点亮，false表示擦除。
 * @return PLATFORM_OK表示成功；对象或图元无效时返回对应错误。
 * @note   函数只修改显存，不刷新设备，不分配内存且不保证线程安全。
 */
int mono_shape_draw_triangle(struct mono_canvas *self,
                             const struct mono_triangle *triangle,
                             bool filled,
                             bool on);

/**
 * @brief  绘制空心或填充圆。
 * @param  self 已初始化的画布对象。
 * @param  circle 圆心和半径，半径必须大于0且不超过INT16_MAX。
 * @param  filled true表示填充，false表示只绘制圆周。
 * @param  on true表示点亮，false表示擦除。
 * @return PLATFORM_OK表示成功；对象或图元无效时返回对应错误。
 * @note   函数只修改显存，不刷新设备，不分配内存且不保证线程安全。
 */
int mono_shape_draw_circle(struct mono_canvas *self,
                           const struct mono_circle *circle,
                           bool filled,
                           bool on);

/**
 * @brief  绘制空心或填充椭圆。
 * @param  self 已初始化的画布对象。
 * @param  ellipse 圆心以及横、纵半轴，半轴必须大于0。
 * @param  filled true表示填充，false表示只绘制边界。
 * @param  on true表示点亮，false表示擦除。
 * @return PLATFORM_OK表示成功；对象或图元无效时返回对应错误。
 * @note   函数只修改显存，不刷新设备，不分配内存且不保证线程安全。
 */
int mono_shape_draw_ellipse(struct mono_canvas *self,
                            const struct mono_ellipse *ellipse,
                            bool filled,
                            bool on);

/**
 * @brief  绘制空心圆弧或填充扇形。
 * @param  self 已初始化的画布对象。
 * @param  arc 圆心、半径和起止角度；右侧为0度，顺时针为正方向。
 * @param  filled true表示填充扇形，false表示只绘制圆弧。
 * @param  on true表示点亮，false表示擦除。
 * @return PLATFORM_OK表示成功；对象或图元无效时返回对应错误。
 * @note   起止角度相同时表示整圆；函数使用浮点反正切判断角度，
 *         只修改显存，不刷新设备且不保证线程安全。
 */
int mono_shape_draw_arc(struct mono_canvas *self,
                        const struct mono_arc *arc,
                        bool filled,
                        bool on);

#endif /* MONO_SHAPE_H */
