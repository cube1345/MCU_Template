/* SPDX-License-Identifier: MIT */
/**
 * @file    imu.c
 * @brief   IMU 抽象设备对象的生命周期和分派包装。
 * @author  weixin
 */
#include "device/imu/imu.h"

#include <stddef.h>

/**
 * @brief  检查 IMU 对象是否已经启动并可访问操作表。
 * @param  self IMU 对象。
 * @return PLATFORM_OK 表示对象可用；其他 PLATFORM_* 表示状态错误。
 */
static int imu_ready_check(const struct imu *self)
{
  if ((self == NULL) || (self->ops == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return PLATFORM_OK;
}

/**
 * @brief  绑定 IMU 基类对象和具体驱动操作表。
 * @param  self 调用者提供的 IMU 基类对象。
 * @param  name 板级逻辑名称，生命周期必须长于 self。
 * @param  model 芯片型号名称，生命周期必须长于 self。
 * @param  ops 具体驱动操作表，start/read 为必选能力。
 * @return PLATFORM_OK 表示绑定成功；其他 PLATFORM_* 表示失败。
 */
int imu_bind(struct imu *self,
             const char *name,
             const char *model,
             const struct imu_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (model == NULL) ||
      (ops == NULL) || (ops->start == NULL) || (ops->read == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((self->ops != NULL) || self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->ops = ops;
  self->name = name;
  self->model = model;
  self->initialized = false;
  imu_reset_data(self);

  return PLATFORM_OK;
}

/**
 * @brief  启动 IMU 硬件。
 * @param  self 已经完成 imu_bind() 的 IMU 对象。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
int imu_start(struct imu *self)
{
  int result;

  if ((self == NULL) || (self->ops == NULL) ||
      (self->ops->start == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_OK;
  }

  result = self->ops->start(self);
  if (result == PLATFORM_OK) {
    self->initialized = true;
    imu_reset_data(self);
  }

  return result;
}

/**
 * @brief  停止 IMU 硬件。
 * @param  self 已经启动的 IMU 对象。
 * @return PLATFORM_OK 表示停止成功；其他 PLATFORM_* 表示失败。
 */
int imu_stop(struct imu *self)
{
  int result;

  result = imu_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (self->ops->stop == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  result = self->ops->stop(self);
  if (result == PLATFORM_OK) {
    self->initialized = false;
  }

  return result;
}

/**
 * @brief  采样一次 IMU 数据。
 * @param  self 已经启动的 IMU 对象。
 * @return PLATFORM_OK 表示采样成功；其他 PLATFORM_* 表示失败。
 */
int imu_read(struct imu *self)
{
  int result;

  result = imu_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (self->ops->read == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->ops->read(self);
}

/**
 * @brief  清空 IMU 最近一次采样缓存。
 * @param  self IMU 对象，允许为 NULL。
 * @return 无返回值。
 */
void imu_reset_data(struct imu *self)
{
  if (self == NULL) {
    return;
  }

  self->data.temperature_c = 0.0f;
  self->data.accel_g.x = 0.0f;
  self->data.accel_g.y = 0.0f;
  self->data.accel_g.z = 0.0f;
  self->data.gyro_dps.x = 0.0f;
  self->data.gyro_dps.y = 0.0f;
  self->data.gyro_dps.z = 0.0f;
  self->data.gyro_rad_s.x = 0.0f;
  self->data.gyro_rad_s.y = 0.0f;
  self->data.gyro_rad_s.z = 0.0f;
}

/**
 * @brief  获取最近一次 IMU 采样数据。
 * @param  self IMU 对象。
 * @return 数据缓存地址；self 为 NULL 时返回 NULL。
 */
const struct imu_data *imu_get_data(const struct imu *self)
{
  if (self == NULL) {
    return NULL;
  }

  return &self->data;
}

/**
 * @brief  获取 IMU 逻辑名称。
 * @param  self IMU 对象。
 * @return 逻辑名称；对象无效时返回 "unknown"。
 */
const char *imu_get_name(const struct imu *self)
{
  if ((self == NULL) || (self->name == NULL)) {
    return "unknown";
  }

  return self->name;
}

/**
 * @brief  获取 IMU 芯片型号名。
 * @param  self IMU 对象。
 * @return 型号名字符串；对象无效时返回 "unknown"。
 */
const char *imu_get_model(const struct imu *self)
{
  if ((self == NULL) || (self->model == NULL)) {
    return "unknown";
  }

  return self->model;
}

/**
 * @brief  查询 IMU 是否已经启动。
 * @param  self IMU 对象。
 * @return true 表示已启动，false 表示未启动或对象无效。
 */
bool imu_is_initialized(const struct imu *self)
{
  return (self != NULL) && self->initialized;
}
