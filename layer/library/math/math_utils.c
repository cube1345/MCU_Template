/* SPDX-License-Identifier: MIT */
/**
 * @file    math_utils.c
 * @brief   嵌入式控制常用数学工具实现。
 * @author  weixin
 */
#include "math_utils.h"

#include "result.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

/**
 * @brief  按从小到大的顺序整理 32 位整数区间。
 * @param  lower：区间下限地址。
 * @param  upper：区间上限地址。
 * @retval None。
 */
static void math_order_i32_range(int32_t *lower, int32_t *upper)
{
  int32_t temporary;

  if (*lower <= *upper) {
    return;
  }

  temporary = *lower;
  *lower = *upper;
  *upper = temporary;
}

/**
 * @brief  按从小到大的顺序整理单精度浮点区间。
 * @param  lower：区间下限地址。
 * @param  upper：区间上限地址。
 * @retval None。
 */
static void math_order_float_range(float *lower, float *upper)
{
  float temporary;

  if (*lower <= *upper) {
    return;
  }

  temporary = *lower;
  *lower = *upper;
  *upper = temporary;
}

/**
 * @brief  判断单精度浮点数是否为有限值。
 * @param  value：待检查的浮点数。
 * @retval true：value 是有限值。
 * @retval false：value 为 NaN 或正负无穷大。
 */
bool math_is_finite(float value)
{
  /*
   * 使用比较实现有限值检查，避免不同嵌入式 C 库对 isfinite 宏的
   * 支持差异。NaN 与自身不相等，无穷大则超出 FLT_MAX。
   */
  return (value == value) && (value <= FLT_MAX) && (value >= -FLT_MAX);
}

/**
 * @brief  返回两个 32 位有符号整数中的较大值。
 * @param  a：第一个整数。
 * @param  b：第二个整数。
 * @return 两个输入中的较大值。
 */
int32_t math_max_i32(int32_t a, int32_t b)
{
  return (a > b) ? a : b;
}

/**
 * @brief  返回两个 32 位有符号整数中的较小值。
 * @param  a：第一个整数。
 * @param  b：第二个整数。
 * @return 两个输入中的较小值。
 */
int32_t math_min_i32(int32_t a, int32_t b)
{
  return (a < b) ? a : b;
}

/**
 * @brief  计算 32 位有符号整数的绝对值。
 * @param  value：待计算的有符号整数。
 * @return 绝对值的无符号表示。
 */
uint32_t math_abs_i32(int32_t value)
{
  if (value >= 0) {
    return (uint32_t)value;
  }

  /* 先加一再取负，避免直接对 INT32_MIN 取负导致有符号溢出。 */
  return (uint32_t)(-(value + 1)) + 1U;
}

/**
 * @brief  返回两个单精度浮点数中的较大值。
 * @param  a：第一个浮点数。
 * @param  b：第二个浮点数。
 * @return 两个输入中的较大值。
 */
float math_max_float(float a, float b)
{
  return (a > b) ? a : b;
}

/**
 * @brief  返回两个单精度浮点数中的较小值。
 * @param  a：第一个浮点数。
 * @param  b：第二个浮点数。
 * @return 两个输入中的较小值。
 */
float math_min_float(float a, float b)
{
  return (a < b) ? a : b;
}

/**
 * @brief  计算单精度浮点数的绝对值。
 * @param  value：待计算的浮点数。
 * @return value 的绝对值。
 */
float math_abs_float(float value)
{
  return (value < 0.0f) ? -value : value;
}

/**
 * @brief  将 32 位有符号整数限制在指定闭区间内。
 * @param  value：待限制的整数。
 * @param  lower：区间下限。
 * @param  upper：区间上限。
 * @return 限幅后的整数。
 */
int32_t math_clamp_i32(int32_t value, int32_t lower, int32_t upper)
{
  math_order_i32_range(&lower, &upper);

  if (value > upper) {
    return upper;
  }

  if (value < lower) {
    return lower;
  }

  return value;
}

/**
 * @brief  将单精度浮点数限制在指定闭区间内。
 * @param  value：待限制的浮点数。
 * @param  lower：区间下限。
 * @param  upper：区间上限。
 * @return 限幅后的浮点数。
 */
float math_clamp_float(float value, float lower, float upper)
{
  math_order_float_range(&lower, &upper);

  if (value > upper) {
    return upper;
  }

  if (value < lower) {
    return lower;
  }

  return value;
}

/**
 * @brief  对 32 位有符号整数应用开区间死区。
 * @param  value：待处理的整数。
 * @param  lower：死区下限。
 * @param  upper：死区上限。
 * @return value 位于死区内部时返回 0，否则返回原值。
 */
int32_t math_deadband_i32(int32_t value, int32_t lower, int32_t upper)
{
  math_order_i32_range(&lower, &upper);
  return ((value > lower) && (value < upper)) ? 0 : value;
}

/**
 * @brief  对单精度浮点数应用开区间死区。
 * @param  value：待处理的浮点数。
 * @param  lower：死区下限。
 * @param  upper：死区上限。
 * @return value 位于死区内部时返回 0.0f，否则返回原值。
 */
float math_deadband_float(float value, float lower, float upper)
{
  math_order_float_range(&lower, &upper);
  return ((value > lower) && (value < upper)) ? 0.0f : value;
}

/**
 * @brief  将弧度角归一化到半开区间 [0, 2PI)。
 * @param  angle_rad：待归一化的弧度角。
 * @param  normalized_rad：用于接收归一化结果的地址。
 * @retval PLATFORM_OK：归一化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输出地址或输入数值无效。
 */
int math_normalize_angle_rad(float angle_rad, float *normalized_rad)
{
  float result;

  if ((normalized_rad == NULL) || (!math_is_finite(angle_rad))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  result = fmodf(angle_rad, MATH_TWO_PI_F);
  if (result < 0.0f) {
    result += MATH_TWO_PI_F;
  }

  *normalized_rad = result;
  return PLATFORM_OK;
}

/**
 * @brief  将弧度角差归一化到半开区间 [-PI, PI)。
 * @param  delta_rad：待归一化的弧度角差。
 * @param  wrapped_rad：用于接收归一化结果的地址。
 * @retval PLATFORM_OK：归一化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输出地址或输入数值无效。
 */
int math_wrap_angle_delta_rad(float delta_rad, float *wrapped_rad)
{
  float result;
  int status;

  if (wrapped_rad == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  status = math_normalize_angle_rad(delta_rad + MATH_PI_F, &result);
  if (status != PLATFORM_OK) {
    return status;
  }

  *wrapped_rad = result - MATH_PI_F;
  return PLATFORM_OK;
}
