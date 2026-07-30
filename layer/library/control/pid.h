/* SPDX-License-Identifier: MIT */
/**
 * @file    pid.h
 * @brief   离散 PID 控制纯算法库。
 * @author  weixin
 *
 * 本模块不访问硬件、不输出日志、不分配内存。调用方负责以稳定周期调用
 * pid_update()，同一个 PID 对象不能由多个任务或中断并发更新。
 */
#ifndef PID_H
#define PID_H

#include <stdbool.h>

/**
 * @brief PID 控制参数。
 */
struct pid_config {
  /* 比例增益。允许为负值，以适配控制方向相反的执行器。 */
  float kp;
  /* 积分增益。积分状态内部已经包含该增益。 */
  float ki;
  /* 微分增益。 */
  float kd;
  /* 最终输出下限，必须小于等于 output_max。 */
  float output_min;
  /* 最终输出上限，必须大于等于 output_min。 */
  float output_max;
  /* 输出绝对值不超过该值时归零，0.0f 表示关闭。 */
  float output_deadband;
  /* 误差绝对值不超过该值时清空积分，0.0f 表示关闭。 */
  float integral_clear_error;
  /* 积分状态下限，必须小于等于 integral_max。 */
  float integral_min;
  /* 积分状态上限，必须大于等于 integral_min。 */
  float integral_max;
};

/**
 * @brief PID 控制器运行状态。
 */
struct pid {
  /* 初始化时复制的控制配置。 */
  struct pid_config config;
  /* 已经包含 ki 增益的积分状态。 */
  float integral;
  /* 上一次误差，用于计算本次微分项。 */
  float previous_error;
  /* 完成初始化后置位，防止使用未初始化对象。 */
  bool initialized;
  /* 获得第一次误差后置位，首次更新时不计算微分项。 */
  bool has_previous_error;
};

/**
 * @brief  初始化 PID 控制器。
 * @param  self：调用方提供的 PID 对象。
 * @param  config：控制参数，初始化期间会被复制到 self。
 * @retval PLATFORM_OK：初始化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：对象或配置地址无效。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：配置内容无效。
 */
int pid_init(struct pid *self, const struct pid_config *config);

/**
 * @brief  清空 PID 积分和历史误差状态。
 * @param  self：PID 对象；传入 NULL 时不执行操作。
 * @retval None。
 * @note   本函数保留初始化配置和 initialized 状态。
 */
void pid_reset(struct pid *self);

/**
 * @brief  根据当前误差更新一次 PID 输出。
 * @param  self：已经初始化的 PID 对象。
 * @param  error：当前误差，通常为目标值减测量值。
 * @param  dt_s：本次更新距离上次更新的时间，单位为秒且必须大于零。
 * @param  output：用于接收限幅后 PID 输出的地址。
 * @retval PLATFORM_OK：计算成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输入或输出地址无效。
 * @retval PLATFORM_ERR_INVALID_STATE：PID 对象尚未初始化。
 */
int pid_update(struct pid *self,
               float error,
               float dt_s,
               float *output);

#endif /* PID_H */
