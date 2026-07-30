/* SPDX-License-Identifier: MIT */
/**
 * @file    encoder_counter.h
 * @brief   Platform硬件正交编码器计数接口。
 * @author  weixin
 *
 * 本接口只表达硬件计数器的生命周期与累计计数，不包含线速度、轮径、
 * 减速比或PID等上层语义。对象由具体MCU后端或Board层静态持有。
 */
#ifndef ENCODER_COUNTER_H
#define ENCODER_COUNTER_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

struct encoder_counter;

struct encoder_counter_ops {
  int (*start)(struct encoder_counter *self);
  int (*stop)(struct encoder_counter *self);
  int (*read)(struct encoder_counter *self, int32_t *count);
  int (*reset)(struct encoder_counter *self);
};

struct encoder_counter {
  const struct encoder_counter_ops *ops;
  const char *name;
  bool initialized;
};

/**
 * @brief  绑定硬件编码器计数器基础对象与MCU后端。
 * @param  self 调用者持有的计数器基础对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  ops MCU后端操作表，所有操作均为必选能力。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数只建立软件关系，不访问HAL或真实硬件。
 */
int encoder_counter_bind(struct encoder_counter *self,
                         const char *name,
                         const struct encoder_counter_ops *ops);

/**
 * @brief  启动硬件正交编码器计数。
 * @param  self 已完成绑定的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   只能在任务或启动上下文调用，不允许在ISR中调用。
 */
int encoder_counter_start(struct encoder_counter *self);

/**
 * @brief  停止硬件正交编码器计数。
 * @param  self 已经启动的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   只能在任务上下文调用；停止后保留最后累计值。
 */
int encoder_counter_stop(struct encoder_counter *self);

/**
 * @brief  读取从最近一次复位开始的有符号累计计数。
 * @param  self 已经启动的计数器对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数不阻塞但会更新后端回绕状态，不支持并发调用。
 */
int encoder_counter_read(struct encoder_counter *self, int32_t *count);

/**
 * @brief  将硬件与软件累计计数同时清零。
 * @param  self 已经启动的计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   复位操作不停止硬件计数，不支持与read并发调用。
 */
int encoder_counter_reset(struct encoder_counter *self);

/**
 * @brief  查询硬件编码器计数器是否已经启动。
 * @param  self 计数器对象。
 * @return true表示已经启动，false表示未启动或对象无效。
 */
bool encoder_counter_is_initialized(
    const struct encoder_counter *self);

#endif /* ENCODER_COUNTER_H */
