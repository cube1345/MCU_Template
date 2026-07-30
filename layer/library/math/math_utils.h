/* SPDX-License-Identifier: MIT */
/**
 * @file    math_utils.h
 * @brief   嵌入式控制常用的无状态数学工具。
 * @author  weixin
 *
 * 本模块不保存状态、不访问硬件，可在任务和中断中调用。调用方必须保证
 * 同一个输出地址不会被多个执行上下文同时写入。
 */
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stdbool.h>
#include <stdint.h>

/* 单精度圆周率，用于弧度角度归一化。 */
#define MATH_PI_F (3.14159265358979323846f)
/* 单精度二倍圆周率，用于弧度角度归一化。 */
#define MATH_TWO_PI_F (6.28318530717958647692f)

/**
 * @brief  判断单精度浮点数是否为有限值。
 * @param  value：待检查的浮点数。
 * @retval true：value 既不是 NaN，也不是正负无穷大。
 * @retval false：value 为 NaN 或正负无穷大。
 */
bool math_is_finite(float value);

/**
 * @brief  返回两个 32 位有符号整数中的较大值。
 * @param  a：第一个整数。
 * @param  b：第二个整数。
 * @return 两个输入中的较大值。
 */
int32_t math_max_i32(int32_t a, int32_t b);

/**
 * @brief  返回两个 32 位有符号整数中的较小值。
 * @param  a：第一个整数。
 * @param  b：第二个整数。
 * @return 两个输入中的较小值。
 */
int32_t math_min_i32(int32_t a, int32_t b);

/**
 * @brief  计算 32 位有符号整数的绝对值。
 * @param  value：待计算的有符号整数。
 * @return 绝对值的无符号表示，可正确表示 INT32_MIN 的绝对值。
 */
uint32_t math_abs_i32(int32_t value);

/**
 * @brief  返回两个单精度浮点数中的较大值。
 * @param  a：第一个浮点数。
 * @param  b：第二个浮点数。
 * @return 两个输入中的较大值；调用方应传入有限浮点数。
 */
float math_max_float(float a, float b);

/**
 * @brief  返回两个单精度浮点数中的较小值。
 * @param  a：第一个浮点数。
 * @param  b：第二个浮点数。
 * @return 两个输入中的较小值；调用方应传入有限浮点数。
 */
float math_min_float(float a, float b);

/**
 * @brief  计算单精度浮点数的绝对值。
 * @param  value：待计算的浮点数。
 * @return value 的绝对值。
 */
float math_abs_float(float value);

/**
 * @brief  将 32 位有符号整数限制在指定闭区间内。
 * @param  value：待限制的整数。
 * @param  lower：区间下限。
 * @param  upper：区间上限。
 * @return 限幅后的整数；上下限传反时会先自动交换。
 */
int32_t math_clamp_i32(int32_t value, int32_t lower, int32_t upper);

/**
 * @brief  将单精度浮点数限制在指定闭区间内。
 * @param  value：待限制的浮点数。
 * @param  lower：区间下限。
 * @param  upper：区间上限。
 * @return 限幅后的浮点数；上下限传反时会先自动交换。
 */
float math_clamp_float(float value, float lower, float upper);

/**
 * @brief  对 32 位有符号整数应用开区间死区。
 * @param  value：待处理的整数。
 * @param  lower：死区下限。
 * @param  upper：死区上限。
 * @return value 位于死区内部时返回 0，否则返回原值。
 */
int32_t math_deadband_i32(int32_t value, int32_t lower, int32_t upper);

/**
 * @brief  对单精度浮点数应用开区间死区。
 * @param  value：待处理的浮点数。
 * @param  lower：死区下限。
 * @param  upper：死区上限。
 * @return value 位于死区内部时返回 0.0f，否则返回原值。
 */
float math_deadband_float(float value, float lower, float upper);

/**
 * @brief  将弧度角归一化到半开区间 [0, 2PI)。
 * @param  angle_rad：待归一化的弧度角。
 * @param  normalized_rad：用于接收归一化结果的地址。
 * @retval PLATFORM_OK：归一化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输出地址或输入数值无效。
 */
int math_normalize_angle_rad(float angle_rad, float *normalized_rad);

/**
 * @brief  将弧度角差归一化到半开区间 [-PI, PI)。
 * @param  delta_rad：待归一化的弧度角差。
 * @param  wrapped_rad：用于接收归一化结果的地址。
 * @retval PLATFORM_OK：归一化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输出地址或输入数值无效。
 */
int math_wrap_angle_delta_rad(float delta_rad, float *wrapped_rad);

#endif /* MATH_UTILS_H */
