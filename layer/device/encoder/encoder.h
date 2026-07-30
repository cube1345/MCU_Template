/* SPDX-License-Identifier: MIT */
/**
 * @file    encoder.h
 * @brief   增量式编码器基础对象与统一操作接口。
 * @author  weixin
 *
 * 编码器是简单设备，基础操作表包含启动、停止、读取累计计数和复位。
 * 速度、距离、轮径及减速比属于组件层，不进入本设备接口。
 */
#ifndef ENCODER_H
#define ENCODER_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

struct encoder;

struct encoder_ops {
  int (*start)(struct encoder *self);
  int (*stop)(struct encoder *self);
  int (*read_count)(struct encoder *self, int32_t *count);
  int (*reset)(struct encoder *self);
};

struct encoder {
  const struct encoder_ops *ops;
  const char *name;
  bool initialized;
};

/**
 * @brief  绑定编码器基础对象与具体驱动操作表。
 * @param  self 调用者持有的编码器对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  ops 具体驱动操作表，所有操作均为必选能力。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数不访问硬件，也不分配堆内存。
 */
int encoder_bind(struct encoder *self,
                 const char *name,
                 const struct encoder_ops *ops);

/**
 * @brief  启动编码器硬件采集。
 * @param  self 已完成绑定的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   只能在任务或启动上下文调用，不允许在ISR中调用。
 */
int encoder_start(struct encoder *self);

/**
 * @brief  停止编码器硬件采集。
 * @param  self 已经启动的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_stop(struct encoder *self);

/**
 * @brief  读取从最近一次复位开始的有符号累计计数。
 * @param  self 已经启动的编码器对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   本函数不阻塞，但同一对象不支持多任务并发读取。
 */
int encoder_read_count(struct encoder *self, int32_t *count);

/**
 * @brief  将编码器累计计数清零。
 * @param  self 已经启动的编码器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int encoder_reset(struct encoder *self);

/**
 * @brief  获取编码器逻辑名称。
 * @param  self 编码器对象。
 * @return 对象名称；对象无效时返回"unknown"。
 */
const char *encoder_get_name(const struct encoder *self);

/**
 * @brief  查询编码器是否已经启动。
 * @param  self 编码器对象。
 * @return true表示已经启动，false表示未启动或对象无效。
 */
bool encoder_is_initialized(const struct encoder *self);

#endif /* ENCODER_H */
