/* SPDX-License-Identifier: MIT */
/**
 * @file    attitude.c
 * @brief   IMU 采样与 Mahony 姿态解算的组合实现。
 * @author  weixin
 */
#include "attitude/attitude.h"

#include "math_utils.h"

#include <stddef.h>

/**
 * @brief  将 IMU 标准数据转换为 Mahony 算法输入。
 * @param  data IMU 最近一次成功采样的数据。
 * @param  accel_g 用于接收三轴加速度，单位为 g。
 * @param  gyro_rad_s 用于接收三轴角速度，单位为 rad/s。
 * @return 无返回值。
 * @note   当前保持 ICM42688 芯片坐标方向，不进行板级轴交换或取反。
 */
static void attitude_make_input(
    const struct imu_data *data,
    struct mahony_vector3f *accel_g,
    struct mahony_vector3f *gyro_rad_s)
{
  accel_g->x = data->accel_g.x;
  accel_g->y = data->accel_g.y;
  accel_g->z = data->accel_g.z;

  gyro_rad_s->x = data->gyro_rad_s.x;
  gyro_rad_s->y = data->gyro_rad_s.y;
  gyro_rad_s->z = data->gyro_rad_s.z;
}

/**
 * @brief  保存 Mahony 输出的角度制欧拉角。
 * @param  self 姿态组件对象。
 * @param  filter_data Mahony 最近一次成功解算的数据。
 * @return 无返回值。
 */
static void attitude_store_data(
    struct attitude *self,
    const struct mahony_data *filter_data)
{
  self->data.roll_deg = filter_data->euler_deg.roll;
  self->data.pitch_deg = filter_data->euler_deg.pitch;
  self->data.yaw_deg = filter_data->euler_deg.yaw;
}

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
                  float sample_period_s)
{
  int result;

  if ((self == NULL) || (imu == NULL) ||
      (!math_is_finite(sample_period_s)) || (sample_period_s <= 0.0f)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if (!imu_is_initialized(imu)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = mahony_init(&self->filter, NULL);
  if (result != PLATFORM_OK) {
    return result;
  }

  self->imu = imu;
  self->data.roll_deg = 0.0f;
  self->data.pitch_deg = 0.0f;
  self->data.yaw_deg = 0.0f;
  self->sample_period_s = sample_period_s;
  self->aligned = false;
  self->initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  采样一次 IMU 并更新姿态解算结果。
 * @param  self 已初始化的姿态组件对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示采样或解算失败。
 * @note   首次调用使用重力方向完成初始对齐；本函数会阻塞等待 I2C 读取，
 *         只能在任务上下文调用，不能在中断中调用。
 */
int attitude_update(struct attitude *self)
{
  const struct imu_data *imu_data;
  const struct mahony_data *filter_data;
  struct mahony_vector3f accel_g;
  struct mahony_vector3f gyro_rad_s;
  int result;

  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized || (self->imu == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = imu_read(self->imu);
  if (result != PLATFORM_OK) {
    return result;
  }

  imu_data = imu_get_data(self->imu);
  if (imu_data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  attitude_make_input(imu_data, &accel_g, &gyro_rad_s);

  if (!self->aligned) {
    result = mahony_align_with_accel(&self->filter, &accel_g);
    if (result == PLATFORM_OK) {
      self->aligned = true;
    }
  } else {
    result = mahony_update(&self->filter, &accel_g, &gyro_rad_s,
                           self->sample_period_s);
  }
  if (result != PLATFORM_OK) {
    return result;
  }

  filter_data = mahony_get_data(&self->filter);
  if (filter_data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  attitude_store_data(self, filter_data);
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次成功解算的欧拉角。
 * @param  self 已初始化的姿态组件对象。
 * @return 姿态数据地址；尚未完成初始对齐或对象无效时返回 NULL。
 * @note   返回值指向 self 内部缓存，不得释放或修改；组件不保证线程安全。
 */
const struct attitude_data *attitude_get_data(
    const struct attitude *self)
{
  if ((self == NULL) || !self->initialized || !self->aligned) {
    return NULL;
  }

  return &self->data;
}

/**
 * @brief  获取最近一次姿态更新使用的 IMU 数据。
 * @param  self 已初始化的姿态组件对象。
 * @return IMU 数据地址；对象无效或尚未完成姿态更新时返回 NULL。
 * @note   本函数不会重新采样或访问 I2C。返回值指向 IMU 内部缓存，
 *         不得释放或修改；组件不保证线程安全。
 */
const struct imu_data *attitude_get_imu_data(
    const struct attitude *self)
{
  if ((self == NULL) || !self->initialized || !self->aligned ||
      (self->imu == NULL)) {
    return NULL;
  }

  return imu_get_data(self->imu);
}

/**
 * @brief  获取最近一次采样的温度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  temperature_c 用于接收摄氏温度的变量。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_temperature(const struct attitude *self,
                             float *temperature_c)
{
  const struct imu_data *imu_data;

  if ((self == NULL) || (temperature_c == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  imu_data = attitude_get_imu_data(self);
  if (imu_data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *temperature_c = imu_data->temperature_c;
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次采样的三轴加速度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  accel_g 用于接收三轴加速度的结构体，单位为 g。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_accel_g(const struct attitude *self,
                         struct imu_vector3f *accel_g)
{
  const struct imu_data *imu_data;

  if ((self == NULL) || (accel_g == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  imu_data = attitude_get_imu_data(self);
  if (imu_data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *accel_g = imu_data->accel_g;
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次采样的三轴角速度。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  gyro_dps 用于接收三轴角速度的结构体，单位为 deg/s。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不访问 I2C，也不保证线程安全。
 */
int attitude_get_gyro_dps(const struct attitude *self,
                          struct imu_vector3f *gyro_dps)
{
  const struct imu_data *imu_data;

  if ((self == NULL) || (gyro_dps == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  imu_data = attitude_get_imu_data(self);
  if (imu_data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *gyro_dps = imu_data->gyro_dps;
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次解算的角度制欧拉角。
 * @param  self 已初始化并完成首次更新的姿态组件对象。
 * @param  euler_deg 用于接收横滚角、俯仰角和偏航角的结构体。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数只复制内部缓存，不更新算法，也不保证线程安全。
 */
int attitude_get_euler_deg(const struct attitude *self,
                           struct attitude_data *euler_deg)
{
  const struct attitude_data *data;

  if ((self == NULL) || (euler_deg == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  data = attitude_get_data(self);
  if (data == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  *euler_deg = *data;
  return PLATFORM_OK;
}
