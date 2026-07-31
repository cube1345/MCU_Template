/* SPDX-License-Identifier: MIT */
/**
 * @file    emm_v5_soft_limit.c
 * @brief   EMM V5速度与位置命令软件限位修饰器实现。
 * @author  weixin
 */
#include "emm_v5_soft_limit.h"

#include "container_of.h"
#include "result.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief  检查修饰器对象是否已经绑定且处于启动状态。
 * @param  self 待检查的软件限位修饰器。
 * @retval PLATFORM_OK表示可访问底层EMM，其他值表示参数或状态错误。
 */
static int emm_v5_soft_limit_ready_check(
    const struct emm_v5_soft_limit *self)
{
  if ((self == NULL) || (self->inner == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->base.initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return PLATFORM_OK;
}

/**
 * @brief  计算给定编码器位置对应的软件限位方向。
 * @param  self 已初始化的软件限位修饰器。
 * @param  position_raw 编码器原始位置。
 * @retval 正限位、负限位或未限位状态。
 */
static enum emm_v5_soft_limit_state emm_v5_soft_limit_calculate_state(
    const struct emm_v5_soft_limit *self,
    int32_t position_raw)
{
  if (position_raw > self->config.position_raw_abs_limit) {
    return EMM_V5_SOFT_LIMIT_POSITIVE;
  }
  if (position_raw < -self->config.position_raw_abs_limit) {
    return EMM_V5_SOFT_LIMIT_NEGATIVE;
  }
  return EMM_V5_SOFT_LIMIT_NONE;
}

/**
 * @brief  将位置命令脉冲换算为编码器原始位置计数。
 * @param  self 已初始化的软件限位修饰器。
 * @param  pulse 有符号位置命令脉冲数。
 * @retval 与pulse对应的编码器原始位置计数。
 */
static int64_t emm_v5_soft_limit_pulse_to_raw(
    const struct emm_v5_soft_limit *self,
    int64_t pulse)
{
  int64_t position_raw;

  position_raw =
      (pulse *
       (int64_t)self->config.encoder_counts_per_revolution) /
      (int64_t)self->config.command_pulses_per_revolution;
  /* reversed改变命令方向，限位判断必须仍以编码器raw坐标为准。 */
  return self->inner->config.reversed ? -position_raw : position_raw;
}

/**
 * @brief  判断目标原始位置是否会继续扩大当前越界绝对值。
 * @param  self 已建立有效位置样本的软件限位修饰器。
 * @param  target_raw 位置命令换算后的目标编码器原始位置。
 * @retval true表示允许转发命令，false表示必须拦截。
 */
static bool emm_v5_soft_limit_target_is_allowed(
    const struct emm_v5_soft_limit *self,
    int64_t target_raw)
{
  int64_t current_abs;
  int64_t target_abs;

  if (self->limit_state == EMM_V5_SOFT_LIMIT_NONE) {
    return true;
  }
  current_abs = (self->position_raw >= 0) ?
                (int64_t)self->position_raw :
                -(int64_t)self->position_raw;
  target_abs = (target_raw >= 0) ? target_raw : -target_raw;
  return target_abs < current_abs;
}

/**
 * @brief  启动被修饰的原始EMM并复位位置样本状态。
 * @param  base 软件限位修饰器的motor基础对象。
 * @retval PLATFORM_OK表示启动成功，其他值由原始EMM返回。
 */
static int emm_v5_soft_limit_start(struct motor *base)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);
  int result;

  result = motor_start(emm_v5_get_motor(self->inner));
  if (result == PLATFORM_OK) {
    /* 新启动周期必须先读取位置，避免使用上次运行留下的限位状态。 */
    self->position_raw = 0;
    self->limit_state = EMM_V5_SOFT_LIMIT_NONE;
    self->position_valid = false;
  }
  return result;
}

/**
 * @brief  停止被修饰的原始EMM并清除位置样本有效标志。
 * @param  base 软件限位修饰器的motor基础对象。
 * @retval PLATFORM_OK表示停止成功，其他值由原始EMM返回。
 */
static int emm_v5_soft_limit_stop(struct motor *base)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);
  int result;

  result = motor_stop(emm_v5_get_motor(self->inner));
  if (result == PLATFORM_OK) {
    self->position_valid = false;
    self->limit_state = EMM_V5_SOFT_LIMIT_NONE;
  }
  return result;
}

/**
 * @brief  检查速度方向后将安全速度命令转发给原始EMM。
 * @param  base 软件限位修饰器的motor基础对象。
 * @param  speed_rpm 有符号目标速度，单位为RPM。
 * @retval PLATFORM_OK表示命令已发送，PLATFORM_ERR_BUSY表示方向被限位拦截。
 */
static int emm_v5_soft_limit_set_speed(
    struct motor *base,
    int32_t speed_rpm)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);
  int64_t raw_direction_speed;

  if (!self->position_valid) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  raw_direction_speed = self->inner->config.reversed ?
                        -(int64_t)speed_rpm : (int64_t)speed_rpm;
  if (((self->limit_state == EMM_V5_SOFT_LIMIT_POSITIVE) &&
       (raw_direction_speed > 0)) ||
      ((self->limit_state == EMM_V5_SOFT_LIMIT_NEGATIVE) &&
       (raw_direction_speed < 0))) {
    return PLATFORM_ERR_BUSY;
  }
  return motor_set_speed(emm_v5_get_motor(self->inner), speed_rpm);
}

/**
 * @brief  读取修饰器最近一次接收的速度命令。
 * @param  base 软件限位修饰器的motor基础对象。
 * @param  speed_rpm 用于接收目标速度的变量。
 * @retval PLATFORM_OK表示读取成功。
 */
static int emm_v5_soft_limit_get_speed(
    struct motor *base,
    int32_t *speed_rpm)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);

  *speed_rpm = self->inner->commanded_speed_rpm;
  return PLATFORM_OK;
}

/**
 * @brief  检查相对位置目标后将安全命令转发给原始EMM。
 * @param  base 软件限位修饰器的motor基础对象。
 * @param  pulse 相对当前位置的有符号脉冲数。
 * @retval PLATFORM_OK表示命令已发送，PLATFORM_ERR_BUSY表示目标被限位拦截。
 */
static int emm_v5_soft_limit_set_position(
    struct motor *base,
    int32_t pulse)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);
  int64_t target_raw;

  if (!self->position_valid) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  target_raw = (int64_t)self->position_raw +
               emm_v5_soft_limit_pulse_to_raw(self, (int64_t)pulse);
  if (!emm_v5_soft_limit_target_is_allowed(self, target_raw)) {
    return PLATFORM_ERR_BUSY;
  }
  return motor_set_position(emm_v5_get_motor(self->inner), pulse);
}

/**
 * @brief  返回最近一次成功读取的编码器原始位置。
 * @param  base 软件限位修饰器的motor基础对象。
 * @param  position_raw 用于接收编码器原始位置的变量。
 * @retval PLATFORM_OK表示样本有效，其他值表示尚未建立位置样本。
 */
static int emm_v5_soft_limit_get_position(
    struct motor *base,
    int32_t *position_raw)
{
  struct emm_v5_soft_limit *self =
      container_of(base, struct emm_v5_soft_limit, base);

  if (!self->position_valid) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  *position_raw = self->position_raw;
  return PLATFORM_OK;
}

/* 软件限位修饰器的生命周期操作表。 */
static const struct motor_ops s_ops = {
  .start = emm_v5_soft_limit_start,
  .stop = emm_v5_soft_limit_stop,
};

/* 软件限位修饰器的速度能力表。 */
static const struct motor_speed_ops s_speed_ops = {
  .set_speed = emm_v5_soft_limit_set_speed,
  .get_speed = emm_v5_soft_limit_get_speed,
};

/* 软件限位修饰器的相对位置能力表。 */
static const struct motor_position_ops s_position_ops = {
  .set_position = emm_v5_soft_limit_set_position,
  .get_position = emm_v5_soft_limit_get_position,
};

/**
 * @brief  初始化EMM V5软件限位修饰器并绑定motor基础对象。
 * @param  self 调用者提供的修饰器对象。
 * @param  name 修饰后motor对象的逻辑名称，生命周期必须长于self。
 * @param  inner 被修饰的原始EMM V5对象，运行期间由self独占。
 * @param  config 限位阈值和位置单位换算配置。
 * @retval PLATFORM_OK表示初始化成功，其他值表示参数、配置或状态错误。
 */
int emm_v5_soft_limit_init(
    struct emm_v5_soft_limit *self,
    const char *name,
    struct emm_v5 *inner,
    const struct emm_v5_soft_limit_config *config)
{
  int result;

  if ((self == NULL) || (name == NULL) || (inner == NULL) ||
      (config == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((config->position_raw_abs_limit <= 0) ||
      (config->encoder_counts_per_revolution == 0U) ||
      (config->command_pulses_per_revolution == 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  if ((self->base.ops != NULL) ||
      motor_is_initialized(emm_v5_get_motor(inner))) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  result = motor_bind(&self->base, name, &s_ops, NULL,
                      &s_speed_ops, &s_position_ops);
  if (result != PLATFORM_OK) {
    return result;
  }
  self->inner = inner;
  self->config = *config;
  self->position_raw = 0;
  self->limit_state = EMM_V5_SOFT_LIMIT_NONE;
  self->position_valid = false;
  return PLATFORM_OK;
}

/**
 * @brief  获取修饰后的motor基础对象。
 * @param  self 已初始化的软件限位修饰器。
 * @retval self有效时返回motor对象，否则返回NULL。
 */
struct motor *emm_v5_soft_limit_get_motor(
    struct emm_v5_soft_limit *self)
{
  return (self != NULL) ? &self->base : NULL;
}

/**
 * @brief  读取实时位置、更新限位状态并在首次越界时停止当前运动。
 * @param  self 已启动的软件限位修饰器。
 * @param  position_raw 用于接收编码器原始位置的变量。
 * @retval PLATFORM_OK表示读取和限位处理成功，其他值表示状态或通信错误。
 */
int emm_v5_soft_limit_position_read(
    struct emm_v5_soft_limit *self,
    int32_t *position_raw)
{
  enum emm_v5_soft_limit_state new_state;
  int result;

  if (position_raw == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = emm_v5_soft_limit_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = emm_v5_read_position(self->inner, position_raw);
  if (result != PLATFORM_OK) {
    return result;
  }

  /* 先保存样本，确保越界停止后仍可判断回退速度和位置命令。 */
  self->position_raw = *position_raw;
  self->position_valid = true;
  new_state = emm_v5_soft_limit_calculate_state(self, *position_raw);
  if ((new_state != EMM_V5_SOFT_LIMIT_NONE) &&
      (new_state != self->limit_state)) {
    result = emm_v5_stop_now(self->inner);
    if (result != PLATFORM_OK) {
      return result;
    }
  }
  self->limit_state = new_state;
  return PLATFORM_OK;
}

/**
 * @brief  将当前位置设置为零点并同步修饰器的位置状态。
 * @param  self 已启动且当前未越界的软件限位修饰器。
 * @retval PLATFORM_OK表示清零成功，PLATFORM_ERR_BUSY表示越界时禁止清零。
 */
int emm_v5_soft_limit_position_clear(
    struct emm_v5_soft_limit *self)
{
  int result;

  result = emm_v5_soft_limit_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (!self->position_valid) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if (self->limit_state != EMM_V5_SOFT_LIMIT_NONE) {
    return PLATFORM_ERR_BUSY;
  }
  result = emm_v5_position_clear(self->inner);
  if (result == PLATFORM_OK) {
    self->position_raw = 0;
    self->limit_state = EMM_V5_SOFT_LIMIT_NONE;
  }
  return result;
}

/**
 * @brief  发送经过软件限位检查的绝对位置命令。
 * @param  self 已启动的软件限位修饰器。
 * @param  pulse 相对驱动器零点的绝对位置脉冲数。
 * @retval PLATFORM_OK表示命令已发送，PLATFORM_ERR_BUSY表示命令会扩大越界。
 */
int emm_v5_soft_limit_move_absolute(
    struct emm_v5_soft_limit *self,
    int32_t pulse)
{
  int64_t target_raw;
  int result;

  result = emm_v5_soft_limit_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (!self->position_valid) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  target_raw = emm_v5_soft_limit_pulse_to_raw(self, (int64_t)pulse);
  if (!emm_v5_soft_limit_target_is_allowed(self, target_raw)) {
    return PLATFORM_ERR_BUSY;
  }
  return emm_v5_move_absolute(self->inner, pulse);
}

/**
 * @brief  立即停止当前运动并保持EMM使能和修饰器启动状态。
 * @param  self 已启动的软件限位修饰器。
 * @retval PLATFORM_OK表示停止命令已发送，其他值表示状态或通信错误。
 */
int emm_v5_soft_limit_motion_stop(
    struct emm_v5_soft_limit *self)
{
  int result;

  result = emm_v5_soft_limit_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  return emm_v5_stop_now(self->inner);
}

/**
 * @brief  获取最近位置样本对应的软件限位状态。
 * @param  self 已初始化的软件限位修饰器。
 * @retval 当前限位方向；对象或位置样本无效时返回EMM_V5_SOFT_LIMIT_NONE。
 */
enum emm_v5_soft_limit_state emm_v5_soft_limit_state_get(
    const struct emm_v5_soft_limit *self)
{
  if ((self == NULL) || !self->position_valid) {
    return EMM_V5_SOFT_LIMIT_NONE;
  }
  return self->limit_state;
}
