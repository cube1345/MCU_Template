/* SPDX-License-Identifier: MIT */
/**
 * @file    attitude.h
 * @brief   IMU 与 Mahony 算法组合形成的姿态解算组件。
 * @author  weixin
 *
 * attitude 组件负责驱动一个已启动的 IMU 对象，并将加速度和角速度
 * 输入 Mahony 算法，最终向应用层提供横滚角、俯仰角和偏航角。
 */
#ifndef ATTITUDE_H
#define ATTITUDE_H

#include "ahrs/mahony.h"
#include "imu/imu.h"

#include <stdbool.h>

struct attitude_data {
  /* 横滚角，单位为度。 */
  float roll_deg;
  /* 俯仰角，单位为度。 */
  float pitch_deg;
  /* 相对偏航角，单位为度；六轴 IMU 无法抑制其长期漂移。 */
  float yaw_deg;
};

struct attitude {
  /* 被组合的 IMU 对象；组件只保存引用，不拥有其存储。 */
  struct imu *imu;
  /* Mahony 算法对象及其内部四元数、反馈和零偏状态。 */
  struct mahony filter;
  /* 最近一次成功解算得到的欧拉角。 */
  struct attitude_data data;
  /* 每次调用 attitude_update() 的期望周期，单位为秒。 */
  float sample_period_s;
  /* 首次使用重力方向完成横滚角和俯仰角对齐后置 true。 */
  bool aligned;
  /* attitude_init() 成功后置 true。 */
  bool initialized;
};

/**
 * @brief  初始化姿态解算组件。
 * @param  self 调用者提供的姿态组件对象。
 * @param  imu 已启动的 IMU 对象，生命周期必须长于 self。
 * @param  sample_period_s attitude_update() 的调用周期，单位为秒。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不分配堆内存，也不访问 IMU 硬件；组件不支持并发调用。
 */
int attitude_init(struct attitude *self,
                  struct imu *imu,
                  float sample_period_s);

/**
 * @brief  采样一次 IMU 并更新姿态解算结果。
 * @param  self 已初始化的姿态组件对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示采样或解算失败。
 * @note   首次调用使用重力方向完成初始对齐；本函数会阻塞等待 I2C 读取，
 *         只能在任务上下文调用，不能在中断中调用。
 */
int attitude_update(struct attitude *self);

/**
 * @brief  获取最近一次成功解算的欧拉角。
 * @param  self 已初始化的姿态组件对象。
 * @return 姿态数据地址；尚未完成初始对齐或对象无效时返回 NULL。
 * @note   返回值指向 self 内部缓存，不得释放或修改；组件不保证线程安全。
 */
const struct attitude_data *attitude_get_data(
    const struct attitude *self);

/**
 * @brief  获取最近一次姿态更新使用的 IMU 数据。
 * @param  self 已初始化的姿态组件对象。
 * @return IMU 数据地址；对象无效或尚未完成姿态更新时返回 NULL。
 * @note   本函数不会重新采样或访问 I2C。返回值指向 IMU 内部缓存，
 *         不得释放或修改；组件不保证线程安全。
 */
const struct imu_data *attitude_get_imu_data(
    const struct attitude *self);

/**
 * @brief  获取最近一次采样的温度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  temperature_c 用于接收摄氏温度的变量。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_temperature(const struct attitude *self,
                             float *temperature_c);

/**
 * @brief  获取最近一次采样的三轴加速度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  accel_g 用于接收三轴加速度的结构体，单位为 g。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_accel_g(const struct attitude *self,
                         struct imu_vector3f *accel_g);

/**
 * @brief  获取最近一次采样的三轴角速度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  gyro_dps 用于接收三轴角速度的结构体，单位为 deg/s。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_gyro_dps(const struct attitude *self,
                          struct imu_vector3f *gyro_dps);

/**
 * @brief  获取最近一次解算的角度制欧拉角。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  euler_deg 用于接收横滚角、俯仰角和偏航角的结构体。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不更新算法，也不保证线程安全。
 */
int attitude_get_euler_deg(const struct attitude *self,
                           struct attitude_data *euler_deg);

#endif /* ATTITUDE_H */
