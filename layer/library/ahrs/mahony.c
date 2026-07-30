/* SPDX-License-Identifier: MIT */
/**
 * @file    mahony.c
 * @brief   Mahony 姿态解算算法实现。
 * @author  weixin
 */
#include "ahrs/mahony.h"

#include "math_utils.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define MAHONY_NORM_EPSILON (1.0e-6f)

/**
 * @brief  检查 Mahony 配置是否由有限数值和合法范围组成。
 * @param  config：待检查的算法配置。
 * @retval true：配置可以安全用于姿态解算。
 * @retval false：配置包含非法数值或范围。
 */
static bool mahony_config_is_valid(const struct mahony_config *config)
{
  if ((config == NULL) ||
      (!math_is_finite(config->kp)) ||
      (!math_is_finite(config->ki)) ||
      (!math_is_finite(config->accel_norm_min_g)) ||
      (!math_is_finite(config->accel_norm_max_g))) {
    return false;
  }

  return (config->kp >= 0.0f) &&
         (config->ki >= 0.0f) &&
         (config->accel_norm_min_g > 0.0f) &&
         (config->accel_norm_min_g <= config->accel_norm_max_g);
}

/**
 * @brief  计算三轴向量模长。
 * @param  vector 三轴向量。
 * @return 向量模长；vector 为 NULL 时返回 0。
 */
static float vector_norm(const struct mahony_vector3f *vector)
{
  if (vector == NULL) {
    return 0.0f;
  }

  return sqrtf((vector->x * vector->x) +
               (vector->y * vector->y) +
               (vector->z * vector->z));
}

/**
 * @brief  原地归一化三轴向量。
 * @param  vector 待归一化的三轴向量。
 * @return true 表示归一化成功，false 表示向量无效或模长过小。
 */
static bool vector_normalize(struct mahony_vector3f *vector)
{
  float norm;

  if (vector == NULL) {
    return false;
  }

  norm = vector_norm(vector);
  if (norm < MAHONY_NORM_EPSILON) {
    return false;
  }

  vector->x /= norm;
  vector->y /= norm;
  vector->z /= norm;

  return true;
}

/**
 * @brief  原地归一化当前姿态四元数。
 * @param  self Mahony 算法对象。
 * @return 无返回值。
 * @note   四元数模长过小时会重置为单位四元数。
 */
static void normalize_quaternion(struct mahony *self)
{
  float norm;
  struct mahony_quaternion *q = &self->data.quaternion;

  norm = sqrtf((q->w * q->w) + (q->x * q->x) +
               (q->y * q->y) + (q->z * q->z));
  if (norm < MAHONY_NORM_EPSILON) {
    q->w = 1.0f;
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
    return;
  }

  q->w /= norm;
  q->x /= norm;
  q->y /= norm;
  q->z /= norm;
}

/**
 * @brief  根据当前四元数更新欧拉角。
 * @param  self Mahony 算法对象。
 * @return 无返回值。
 * @note   同时更新 rad 和 deg 两种单位，方便算法和调试使用。
 */
static void update_euler(struct mahony *self)
{
  float sin_pitch;
  float roll_rad;
  float pitch_rad;
  float yaw_rad;
  const float w = self->data.quaternion.w;
  const float x = self->data.quaternion.x;
  const float y = self->data.quaternion.y;
  const float z = self->data.quaternion.z;

  roll_rad = atan2f(2.0f * ((w * x) + (y * z)),
                    1.0f - 2.0f * ((x * x) + (y * y)));
  sin_pitch = 2.0f * ((w * y) - (z * x));
  sin_pitch = math_clamp_float(sin_pitch, -1.0f, 1.0f);
  pitch_rad = asinf(sin_pitch);
  yaw_rad = atan2f(2.0f * ((w * z) + (x * y)),
                   1.0f - 2.0f * ((y * y) + (z * z)));

  self->data.euler_rad.roll = roll_rad;
  self->data.euler_rad.pitch = pitch_rad;
  self->data.euler_rad.yaw = yaw_rad;
  self->data.euler_deg.roll = roll_rad * MAHONY_RAD_TO_DEG;
  self->data.euler_deg.pitch = pitch_rad * MAHONY_RAD_TO_DEG;
  self->data.euler_deg.yaw = yaw_rad * MAHONY_RAD_TO_DEG;
}

/**
 * @brief  判断当前加速度模长是否允许参与反馈。
 * @param  self Mahony 算法对象。
 * @param  accel 三轴加速度，单位 g。
 * @return true 表示可用于姿态反馈，false 表示应忽略。
 */
static bool accel_feedback_allowed(const struct mahony *self,
                                   const struct mahony_vector3f *accel)
{
  const float norm = vector_norm(accel);

  return (norm >= self->config.accel_norm_min_g) &&
         (norm <= self->config.accel_norm_max_g);
}

/**
 * @brief  使用加速度方向对角速度进行 Mahony 反馈修正。
 * @param  self Mahony 算法对象。
 * @param  accel_g 三轴加速度，单位 g。
 * @param  gyro_rad_s 输入/输出角速度，单位 rad/s。
 * @param  dt_s 采样周期，单位 s。
 * @return 无返回值。
 */
static void apply_feedback(struct mahony *self,
                           const struct mahony_vector3f *accel_g,
                           struct mahony_vector3f *gyro_rad_s,
                           float dt_s)
{
  struct mahony_vector3f accel;
  float half_ex;
  float half_ey;
  float half_ez;
  const float q0 = self->data.quaternion.w;
  const float q1 = self->data.quaternion.x;
  const float q2 = self->data.quaternion.y;
  const float q3 = self->data.quaternion.z;
  const float half_vx = (q1 * q3) - (q0 * q2);
  const float half_vy = (q0 * q1) + (q2 * q3);
  const float half_vz = (q0 * q0) - 0.5f + (q3 * q3);

  self->data.accel_feedback_valid = 0U;
  if ((accel_g == NULL) || (gyro_rad_s == NULL) || (dt_s <= 0.0f)) {
    return;
  }

  accel = *accel_g;
  if (!accel_feedback_allowed(self, &accel) ||
      !vector_normalize(&accel)) {
    return;
  }

  half_ex = (accel.y * half_vz) - (accel.z * half_vy);
  half_ey = (accel.z * half_vx) - (accel.x * half_vz);
  half_ez = (accel.x * half_vy) - (accel.y * half_vx);

  if (self->config.ki > 0.0f) {
    self->data.integral_error.x += self->config.ki * half_ex * dt_s;
    self->data.integral_error.y += self->config.ki * half_ey * dt_s;
    self->data.integral_error.z += self->config.ki * half_ez * dt_s;
  } else {
    self->data.integral_error.x = 0.0f;
    self->data.integral_error.y = 0.0f;
    self->data.integral_error.z = 0.0f;
  }

  gyro_rad_s->x += self->data.integral_error.x +
                   (self->config.kp * half_ex);
  gyro_rad_s->y += self->data.integral_error.y +
                   (self->config.kp * half_ey);
  gyro_rad_s->z += self->data.integral_error.z +
                   (self->config.kp * half_ez);
  self->data.accel_feedback_valid = 1U;
}

/**
 * @brief  根据角速度积分更新四元数。
 * @param  self Mahony 算法对象。
 * @param  gyro_rad_s 三轴角速度，单位 rad/s。
 * @param  dt_s 采样周期，单位 s。
 * @return 无返回值。
 */
static void integrate(struct mahony *self,
                      const struct mahony_vector3f *gyro_rad_s,
                      float dt_s)
{
  const float half_dt = 0.5f * dt_s;
  const float q0 = self->data.quaternion.w;
  const float q1 = self->data.quaternion.x;
  const float q2 = self->data.quaternion.y;
  const float q3 = self->data.quaternion.z;

  if ((gyro_rad_s == NULL) || (dt_s <= 0.0f)) {
    return;
  }

  self->data.quaternion.w += (-q1 * gyro_rad_s->x -
                              q2 * gyro_rad_s->y -
                              q3 * gyro_rad_s->z) * half_dt;
  self->data.quaternion.x += (q0 * gyro_rad_s->x +
                              q2 * gyro_rad_s->z -
                              q3 * gyro_rad_s->y) * half_dt;
  self->data.quaternion.y += (q0 * gyro_rad_s->y -
                              q1 * gyro_rad_s->z +
                              q3 * gyro_rad_s->x) * half_dt;
  self->data.quaternion.z += (q0 * gyro_rad_s->z +
                              q1 * gyro_rad_s->y -
                              q2 * gyro_rad_s->x) * half_dt;

  normalize_quaternion(self);
}

/**
 * @brief  获取 Mahony 默认算法配置。
 * @param  config 用于接收默认配置的结构体。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int mahony_get_default_config(struct mahony_config *config)
{
  if (config == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  config->kp = 2.0f;
  config->ki = 0.05f;
  config->accel_norm_min_g = 0.5f;
  config->accel_norm_max_g = 1.5f;

  return PLATFORM_OK;
}

/**
 * @brief  初始化 Mahony 算法对象。
 * @param  self 调用者提供的算法对象。
 * @param  config 算法配置；传 NULL 时使用默认配置。
 * @return PLATFORM_OK 表示初始化成功；其他 PLATFORM_* 表示失败。
 */
int mahony_init(struct mahony *self, const struct mahony_config *config)
{
  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if (config != NULL) {
    if (!mahony_config_is_valid(config)) {
      return PLATFORM_ERR_INVALID_CONFIGURATION;
    }
    self->config = *config;
  } else {
    (void)mahony_get_default_config(&self->config);
  }

  mahony_reset(self);
  return PLATFORM_OK;
}

/**
 * @brief  复位 Mahony 算法内部状态。
 * @param  self Mahony 算法对象，允许为 NULL。
 * @return 无返回值。
 */
void mahony_reset(struct mahony *self)
{
  if (self == NULL) {
    return;
  }

  self->data.quaternion.w = 1.0f;
  self->data.quaternion.x = 0.0f;
  self->data.quaternion.y = 0.0f;
  self->data.quaternion.z = 0.0f;
  self->data.euler_rad.roll = 0.0f;
  self->data.euler_rad.pitch = 0.0f;
  self->data.euler_rad.yaw = 0.0f;
  self->data.euler_deg.roll = 0.0f;
  self->data.euler_deg.pitch = 0.0f;
  self->data.euler_deg.yaw = 0.0f;
  self->data.gyro_bias_rad_s.x = 0.0f;
  self->data.gyro_bias_rad_s.y = 0.0f;
  self->data.gyro_bias_rad_s.z = 0.0f;
  self->data.gyro_bias_dps.x = 0.0f;
  self->data.gyro_bias_dps.y = 0.0f;
  self->data.gyro_bias_dps.z = 0.0f;
  self->data.integral_error.x = 0.0f;
  self->data.integral_error.y = 0.0f;
  self->data.integral_error.z = 0.0f;
  self->data.accel_feedback_valid = 0U;
}

/**
 * @brief  使用静止加速度方向初始化横滚和俯仰。
 * @param  self Mahony 算法对象。
 * @param  accel_g 三轴加速度，单位 g。
 * @return PLATFORM_OK 表示对齐成功；其他 PLATFORM_* 表示失败。
 */
int mahony_align_with_accel(struct mahony *self,
                            const struct mahony_vector3f *accel_g)
{
  struct mahony_vector3f accel;
  float roll_rad;
  float pitch_rad;
  float half_roll;
  float half_pitch;

  if ((self == NULL) || (accel_g == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  accel = *accel_g;
  if (!accel_feedback_allowed(self, &accel) ||
      !vector_normalize(&accel)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  roll_rad = atan2f(accel.y, accel.z);
  pitch_rad = atan2f(-accel.x,
                     sqrtf((accel.y * accel.y) + (accel.z * accel.z)));
  half_roll = roll_rad * 0.5f;
  half_pitch = pitch_rad * 0.5f;

  self->data.quaternion.w = cosf(half_roll) * cosf(half_pitch);
  self->data.quaternion.x = sinf(half_roll) * cosf(half_pitch);
  self->data.quaternion.y = cosf(half_roll) * sinf(half_pitch);
  self->data.quaternion.z = -sinf(half_roll) * sinf(half_pitch);

  normalize_quaternion(self);
  update_euler(self);
  return PLATFORM_OK;
}

/**
 * @brief  更新一次 Mahony 姿态解算。
 * @param  self Mahony 算法对象。
 * @param  accel_g 三轴加速度，单位 g。
 * @param  gyro_rad_s 三轴角速度，单位 rad/s。
 * @param  dt_s 采样周期，单位 s。
 * @return PLATFORM_OK 表示更新成功；其他 PLATFORM_* 表示失败。
 */
int mahony_update(struct mahony *self,
                  const struct mahony_vector3f *accel_g,
                  const struct mahony_vector3f *gyro_rad_s,
                  float dt_s)
{
  struct mahony_vector3f gyro;

  if ((self == NULL) || (accel_g == NULL) || (gyro_rad_s == NULL) ||
      (!math_is_finite(dt_s)) || (dt_s <= 0.0f)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  gyro.x = gyro_rad_s->x - self->data.gyro_bias_rad_s.x;
  gyro.y = gyro_rad_s->y - self->data.gyro_bias_rad_s.y;
  gyro.z = gyro_rad_s->z - self->data.gyro_bias_rad_s.z;

  apply_feedback(self, accel_g, &gyro, dt_s);
  integrate(self, &gyro, dt_s);
  update_euler(self);

  return PLATFORM_OK;
}

/**
 * @brief  设置陀螺零偏。
 * @param  self Mahony 算法对象，允许为 NULL。
 * @param  gyro_bias_rad_s 三轴陀螺零偏，单位 rad/s。
 * @return 无返回值。
 */
void mahony_set_gyro_bias(struct mahony *self,
                          const struct mahony_vector3f *gyro_bias_rad_s)
{
  if ((self == NULL) || (gyro_bias_rad_s == NULL)) {
    return;
  }

  self->data.gyro_bias_rad_s = *gyro_bias_rad_s;
  self->data.gyro_bias_dps.x = gyro_bias_rad_s->x * MAHONY_RAD_TO_DEG;
  self->data.gyro_bias_dps.y = gyro_bias_rad_s->y * MAHONY_RAD_TO_DEG;
  self->data.gyro_bias_dps.z = gyro_bias_rad_s->z * MAHONY_RAD_TO_DEG;
}

/**
 * @brief  获取 Mahony 当前输出和内部状态。
 * @param  self Mahony 算法对象。
 * @return 数据结构地址；self 为 NULL 时返回 NULL。
 */
const struct mahony_data *mahony_get_data(const struct mahony *self)
{
  if (self == NULL) {
    return NULL;
  }

  return &self->data;
}
