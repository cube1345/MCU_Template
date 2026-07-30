/* SPDX-License-Identifier: MIT */
/**
 * @file    imu.h
 * @brief   IMU 抽象设备对象和统一采样接口。
 * @author  weixin
 *
 * IMU 是中等复杂度设备：device 层只描述“一个惯性测量单元”应有的
 * 生命周期和采样能力，具体芯片寄存器、总线和地址留给 driver/board 层。
 */
#ifndef IMU_H
#define IMU_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

#define IMU_DEG_TO_RAD (0.01745329251994329577f)

struct imu;

struct imu_vector3f {
  /* 三轴向量的 X 轴分量。 */
  float x;
  /* 三轴向量的 Y 轴分量。 */
  float y;
  /* 三轴向量的 Z 轴分量。 */
  float z;
};

struct imu_data {
  /* 芯片内部温度，单位摄氏度。 */
  float temperature_c;
  /* 三轴加速度，单位 g。 */
  struct imu_vector3f accel_g;
  /* 三轴角速度，单位 deg/s，方便人工调试。 */
  struct imu_vector3f gyro_dps;
  /* 三轴角速度，单位 rad/s，方便姿态算法直接使用。 */
  struct imu_vector3f gyro_rad_s;
};

struct imu_ops {
  /* 启动具体 IMU 芯片，通常会做 ID 检查、复位和寄存器配置。 */
  int (*start)(struct imu *self);
  /* 停止具体 IMU 芯片；不支持时可填 NULL。 */
  int (*stop)(struct imu *self);
  /* 采样一次，并把结果写入 struct imu::data。 */
  int (*read)(struct imu *self);
};

struct imu {
  /* 具体驱动绑定的操作表；应用层不要直接访问。 */
  const struct imu_ops *ops;
  /* 板级给这个对象起的逻辑名，例如 imu0。 */
  const char *name;
  /* 具体芯片型号名，例如 ICM42688。 */
  const char *model;
  /* start 成功后置 true，stop 成功后置 false。 */
  bool initialized;
  /* 最近一次采样的数据缓存。 */
  struct imu_data data;
};

/**
 * @brief  绑定 IMU 基类对象和具体驱动操作表。
 * @param  self 调用者提供的 IMU 基类对象，生命周期由调用者管理。
 * @param  name 板级逻辑名，字符串必须在对象生命周期内有效。
 * @param  model 芯片型号名，字符串必须在对象生命周期内有效。
 * @param  ops 具体驱动操作表，start/read 为必选能力。
 * @return PLATFORM_OK 表示绑定成功；其他 PLATFORM_* 表示错误。
 * @note   本函数只绑定对象，不访问硬件；重复绑定会返回无效状态。
 */
int imu_bind(struct imu *self,
             const char *name,
             const char *model,
             const struct imu_ops *ops);

/**
 * @brief  启动 IMU 硬件。
 * @param  self 已经完成 imu_bind() 的 IMU 对象。
 * @return PLATFORM_OK 表示启动成功；失败时透传具体驱动错误码。
 * @note   可能发生阻塞 I2C 访问，不建议在中断上下文调用。
 */
int imu_start(struct imu *self);

/**
 * @brief  停止 IMU 硬件。
 * @param  self 已经启动的 IMU 对象。
 * @return PLATFORM_OK 表示停止成功；不支持 stop 时返回
 *         PLATFORM_ERR_UNSUPPORTED。
 */
int imu_stop(struct imu *self);

/**
 * @brief  采样一次 IMU 数据。
 * @param  self 已经启动的 IMU 对象。
 * @return PLATFORM_OK 表示采样成功；失败时保留上一次数据。
 * @note   采样结果可通过 imu_get_data() 获取。
 */
int imu_read(struct imu *self);

/**
 * @brief  清空 IMU 最近一次采样缓存。
 * @param  self IMU 对象，允许传入 NULL，此时函数直接返回。
 * @note   本函数不访问硬件，只修改对象内的数据缓存。
 */
void imu_reset_data(struct imu *self);

/**
 * @brief  获取最近一次 IMU 采样数据。
 * @param  self IMU 对象。
 * @return 数据缓存地址；self 为 NULL 时返回 NULL。
 */
const struct imu_data *imu_get_data(const struct imu *self);

/**
 * @brief  获取 IMU 逻辑名。
 * @param  self IMU 对象。
 * @return 逻辑名字符串；对象无效时返回 "unknown"。
 */
const char *imu_get_name(const struct imu *self);

/**
 * @brief  获取 IMU 芯片型号名。
 * @param  self IMU 对象。
 * @return 型号名字符串；对象无效时返回 "unknown"。
 */
const char *imu_get_model(const struct imu *self);

/**
 * @brief  查询 IMU 是否已经启动。
 * @param  self IMU 对象。
 * @return true 表示已经启动，false 表示未启动或对象无效。
 */
bool imu_is_initialized(const struct imu *self);

#endif /* IMU_H */
