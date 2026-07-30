/* SPDX-License-Identifier: MIT */
/**
 * @file    low_pass_filter.h
 * @brief   一阶低通滤波纯算法库。
 * @author  weixin
 *
 * 本模块不访问硬件、不分配内存。调用方负责提供并长期持有滤波器对象，
 * 同一个对象不能由多个任务或中断并发更新。
 */
#ifndef LOW_PASS_FILTER_H
#define LOW_PASS_FILTER_H

#include <stdbool.h>

/**
 * @brief 一阶低通滤波器运行状态。
 */
struct low_pass_filter {
  /* 新输入的权重，合法范围为 0.0f 至 1.0f。 */
  float alpha;
  /* 最近一次有效的滤波输出。 */
  float output;
  /* 完成初始化后置位，防止使用未初始化对象。 */
  bool initialized;
  /* 产生第一个输出后置位，用于避免从零开始产生人为过渡。 */
  bool has_output;
};

/**
 * @brief  初始化一阶低通滤波器。
 * @param  self：调用方提供的滤波器对象。
 * @param  alpha：新输入权重，合法范围为 0.0f 至 1.0f。
 * @retval PLATFORM_OK：初始化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：self 为 NULL。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：alpha 非法。
 */
int low_pass_filter_init(struct low_pass_filter *self, float alpha);

/**
 * @brief  使用指定值复位滤波输出。
 * @param  self：已经初始化的滤波器对象。
 * @param  value：复位后的滤波输出，必须是有限浮点数。
 * @retval PLATFORM_OK：复位成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未初始化。
 */
int low_pass_filter_reset(struct low_pass_filter *self, float value);

/**
 * @brief  输入一个新样本并计算滤波结果。
 * @param  self：已经初始化的滤波器对象。
 * @param  input：当前输入样本，必须是有限浮点数。
 * @param  output：用于接收本次滤波结果的地址。
 * @retval PLATFORM_OK：计算成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未初始化。
 * @note   第一个有效样本会直接成为输出，避免从零开始缓慢过渡。
 */
int low_pass_filter_update(struct low_pass_filter *self,
                           float input,
                           float *output);

/**
 * @brief  获取当前滤波输出。
 * @param  self：已经初始化并至少更新或复位一次的滤波器对象。
 * @param  output：用于接收当前滤波输出的地址。
 * @retval PLATFORM_OK：读取成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：参数无效。
 * @retval PLATFORM_ERR_INVALID_STATE：对象尚未产生有效输出。
 */
int low_pass_filter_get_output(const struct low_pass_filter *self,
                               float *output);

#endif /* LOW_PASS_FILTER_H */
