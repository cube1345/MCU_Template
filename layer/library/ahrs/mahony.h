/* SPDX-License-Identifier: MIT */
/**
 * @file    mahony.h
 * @brief   Mahony 姿态解算纯算法库。
 * @author  weixin
 *
 * 本文件不依赖 IMU 设备、I2C 总线或 MCU 平台。调用方只需要传入
 * accel_g、gyro_rad_s 和 dt_s，就能更新姿态四元数与欧拉角。
 */
#ifndef MAHONY_H
#define MAHONY_H

#include "result.h"

#include <stdint.h>

#define MAHONY_RAD_TO_DEG (57.2957795130823208768f)

struct mahony_vector3f {
  float x;
  float y;
  float z;
};

struct mahony_quaternion {
  float w;
  float x;
  float y;
  float z;
};

struct mahony_euler {
  float roll;
  float pitch;
  float yaw;
};

struct mahony_config {
  /* 比例反馈系数，越大姿态收敛越快，但过大容易抖动。 */
  float kp;
  /* 积分反馈系数，用于慢速补偿陀螺零偏。 */
  float ki;
  /* 允许参与反馈的最小加速度模长，单位 g。 */
  float accel_norm_min_g;
  /* 允许参与反馈的最大加速度模长，单位 g。 */
  float accel_norm_max_g;
};

struct mahony_data {
  struct mahony_quaternion quaternion;
  struct mahony_euler euler_rad;
  struct mahony_euler euler_deg;
  struct mahony_vector3f gyro_bias_rad_s;
  struct mahony_vector3f gyro_bias_dps;
  struct mahony_vector3f integral_error;
  uint8_t accel_feedback_valid;
};

struct mahony {
  /* 算法配置。 */
  struct mahony_config config;
  /* 算法状态和输出数据。 */
  struct mahony_data data;
};

/**
 * @brief  获取 Mahony 默认算法配置。
 * @param  config 用于接收默认配置的结构体。
 * @return PLATFORM_OK 表示成功；config 为 NULL 时返回参数错误。
 */
int mahony_get_default_config(struct mahony_config *config);

/**
 * @brief  初始化 Mahony 算法对象。
 * @param  self 调用者提供的算法对象。
 * @param  config 算法配置；传 NULL 时使用默认配置。
 * @return PLATFORM_OK 表示成功；失败时返回 PLATFORM_* 错误码。
 * @note   本算法不持有外部资源，也不访问硬件。
 */
int mahony_init(struct mahony *self, const struct mahony_config *config);

/**
 * @brief  复位姿态、积分误差和零偏状态。
 * @param  self Mahony 算法对象，允许为 NULL。
 */
void mahony_reset(struct mahony *self);

/**
 * @brief  使用静止状态下的加速度方向初始化横滚和俯仰。
 * @param  self Mahony 算法对象。
 * @param  accel_g 三轴加速度，单位 g。
 * @return PLATFORM_OK 表示对齐成功；加速度无效时返回配置错误。
 */
int mahony_align_with_accel(struct mahony *self,
                            const struct mahony_vector3f *accel_g);

/**
 * @brief  更新一次姿态解算。
 * @param  self Mahony 算法对象。
 * @param  accel_g 三轴加速度，单位 g。
 * @param  gyro_rad_s 三轴角速度，单位 rad/s。
 * @param  dt_s 采样周期，单位 s。
 * @return PLATFORM_OK 表示更新成功；参数无效时返回错误码。
 */
int mahony_update(struct mahony *self,
                  const struct mahony_vector3f *accel_g,
                  const struct mahony_vector3f *gyro_rad_s,
                  float dt_s);

/**
 * @brief  设置陀螺零偏。
 * @param  self Mahony 算法对象，允许为 NULL。
 * @param  gyro_bias_rad_s 三轴陀螺零偏，单位 rad/s，允许为 NULL。
 */
void mahony_set_gyro_bias(struct mahony *self,
                          const struct mahony_vector3f *gyro_bias_rad_s);

/**
 * @brief  获取 Mahony 当前姿态和内部状态。
 * @param  self Mahony 算法对象。
 * @return 数据结构地址；self 为 NULL 时返回 NULL。
 */
const struct mahony_data *mahony_get_data(const struct mahony *self);

#endif /* MAHONY_H */
