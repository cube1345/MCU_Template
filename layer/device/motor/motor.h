/* SPDX-License-Identifier: MIT */
/**
 * @file    motor.h
 * @brief   电机类执行器基础对象与可选能力接口。
 * @author  weixin
 *
 * motor 是电机类设备的公共抽象，只固定生命周期能力。输出、速度和位置
 * 通过独立能力表挂载，普通直流电机、舵机、闭环电机轴可以按需实现，
 * 应用层仍然只依赖 struct motor * 和本文件提供的统一包装函数。
 */
#ifndef MOTOR_H
#define MOTOR_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_OUTPUT_MIN INT16_C(-1000)
#define MOTOR_OUTPUT_MAX INT16_C(1000)
#define MOTOR_OUTPUT_STOP INT16_C(0)

struct motor;

struct motor_ops {
  int (*start)(struct motor *self);
  int (*stop)(struct motor *self);
};

struct motor_output_ops {
  int (*set_output)(struct motor *self, int16_t output);
  int (*coast)(struct motor *self);
  int (*brake)(struct motor *self);
};

struct motor_speed_ops {
  int (*set_speed)(struct motor *self, int32_t speed);
  int (*get_speed)(struct motor *self, int32_t *speed);
};

struct motor_position_ops {
  int (*set_position)(struct motor *self, int32_t position);
  int (*get_position)(struct motor *self, int32_t *position);
};

struct motor {
  const struct motor_ops *ops;
  const struct motor_output_ops *output_ops;
  const struct motor_speed_ops *speed_ops;
  const struct motor_position_ops *position_ops;
  const char *name;
  bool initialized;
};

/**
 * @brief  绑定电机基础对象、生命周期表和可选能力表。
 * @param  self 调用者持有的电机对象。
 * @param  name 逻辑名称，生命周期必须长于 self。
 * @param  ops 生命周期操作表，start 和 stop 必须有效。
 * @param  output_ops 输出能力表；不支持输出能力时传入 NULL。
 * @param  speed_ops 速度能力表；不支持速度能力时传入 NULL。
 * @param  position_ops 位置能力表；不支持位置能力时传入 NULL。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只建立软件对象关系，不访问硬件或分配堆内存。
 */
int motor_bind(struct motor *self,
               const char *name,
               const struct motor_ops *ops,
               const struct motor_output_ops *output_ops,
               const struct motor_speed_ops *speed_ops,
               const struct motor_position_ops *position_ops);

/**
 * @brief  启动电机设备的底层资源。
 * @param  self 已完成绑定的电机对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   对普通直流电机通常表示启动 PWM；对舵机可能表示启动输出定时器。
 */
int motor_start(struct motor *self);

/**
 * @brief  停止电机设备的底层资源。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int motor_stop(struct motor *self);

/**
 * @brief  设置开环输出千分比。
 * @param  self 已经启动的电机对象。
 * @param  output 范围 -1000 到 1000；负数反向，0 滑行，正数正向。
 * @return PLATFORM_OK 表示成功；不支持输出能力时返回 PLATFORM_ERR_UNSUPPORTED。
 * @note   本函数适合普通直流电机，不提供速度闭环。
 */
int motor_set_output(struct motor *self, int16_t output);

/**
 * @brief  让电机进入滑行状态。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；不支持输出能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_coast(struct motor *self);

/**
 * @brief  让电机进入电气制动状态。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；不支持制动能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_brake(struct motor *self);

/**
 * @brief  设置目标速度。
 * @param  self 已经启动的电机对象。
 * @param  speed 目标速度，单位由具体驱动或组件在自身头文件中定义。
 * @return PLATFORM_OK 表示成功；不支持速度能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_set_speed(struct motor *self, int32_t speed);

/**
 * @brief  获取当前速度。
 * @param  self 已经启动的电机对象。
 * @param  speed 用于接收当前速度的变量指针。
 * @return PLATFORM_OK 表示成功；不支持速度能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_get_speed(struct motor *self, int32_t *speed);

/**
 * @brief  设置目标位置。
 * @param  self 已经启动的电机对象。
 * @param  position 目标位置，单位由具体驱动或组件在自身头文件中定义。
 * @return PLATFORM_OK 表示成功；不支持位置能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_set_position(struct motor *self, int32_t position);

/**
 * @brief  获取当前位置。
 * @param  self 已经启动的电机对象。
 * @param  position 用于接收当前位置的变量指针。
 * @return PLATFORM_OK 表示成功；不支持位置能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_get_position(struct motor *self, int32_t *position);

/**
 * @brief  查询电机是否支持开环输出能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_output(const struct motor *self);

/**
 * @brief  查询电机是否支持速度能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_speed(const struct motor *self);

/**
 * @brief  查询电机是否支持位置能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_position(const struct motor *self);

/**
 * @brief  获取电机逻辑名称。
 * @param  self 电机对象。
 * @return 对象名称；对象无效时返回 "unknown"。
 */
const char *motor_get_name(const struct motor *self);

/**
 * @brief  查询电机底层资源是否已经启动。
 * @param  self 电机对象。
 * @return true 表示已经启动；false 表示未启动或对象无效。
 */
bool motor_is_initialized(const struct motor *self);

#endif /* MOTOR_H */
