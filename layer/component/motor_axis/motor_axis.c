/* SPDX-License-Identifier: MIT */
/**
 * @file    motor_axis.c
 * @brief   闭环电机轴的模式切换、编码器采样与串级控制实现。
 * @author  weixin
 */
#include "motor_axis.h"

#include "container_of.h"
#include "math_utils.h"
#include "result.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* 无闭环控制，输出保持为调用者最后一次明确设置的状态。 */
#define MOTOR_AXIS_MODE_OPEN_LOOP UINT8_C(0)
/* 速度 PI 控制模式。 */
#define MOTOR_AXIS_MODE_SPEED UINT8_C(1)
/* 位置 P 外环与速度 PI 内环组成的串级控制模式。 */
#define MOTOR_AXIS_MODE_POSITION UINT8_C(2)
/* 增益上限用于拒绝无穷大和明显错误的配置值。 */
#define MOTOR_AXIS_GAIN_MAX (1000.0f)
/* 毫秒转换为秒时使用的比例常量。 */
#define MOTOR_AXIS_MS_PER_SECOND (1000.0f)
/* 计算计数每秒时使用的整数比例常量。 */
#define MOTOR_AXIS_COUNTS_PER_SECOND_SCALE INT64_C(1000)

/**
 * @brief  将 64 位计算结果饱和转换为 32 位有符号整数。
 * @param  value 待转换的 64 位数值。
 * @return 位于 INT32_MIN 至 INT32_MAX 范围内的结果。
 */
static int32_t motor_axis_saturate_int32(int64_t value)
{
  if (value > INT32_MAX) {
    return INT32_MAX;
  }
  if (value < INT32_MIN) {
    return INT32_MIN;
  }

  return (int32_t)value;
}

/**
 * @brief  将限幅后的浮点输出四舍五入为电机输出千分比。
 * @param  value 已限制在 int16_t 有效范围内的浮点输出。
 * @return 四舍五入后的电机输出。
 */
static int16_t motor_axis_round_output(float value)
{
  if (value >= 0.0f) {
    return (int16_t)(value + 0.5f);
  }

  return (int16_t)(value - 0.5f);
}

/**
 * @brief  清除闭环目标和速度积分状态。
 * @param  self 电机轴对象。
 * @return 无返回值。
 */
static void motor_axis_reset_control(struct motor_axis *self)
{
  self->target_speed = 0;
  self->target_position = self->current_position;
  self->speed_integral = 0.0f;
}

/**
 * @brief  校验电机轴控制配置是否安全且可计算。
 * @param  config 待校验的配置。
 * @return PLATFORM_OK 表示有效；否则返回配置错误。
 */
static int motor_axis_config_check(
    const struct motor_axis_config *config)
{
  if ((config == NULL) || (config->update_period_ms == 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  if ((!math_is_finite(config->speed_filter_alpha)) ||
      (config->speed_filter_alpha < 0.0f) ||
      (config->speed_filter_alpha > 1.0f) ||
      (!math_is_finite(config->speed_kp)) ||
      (!math_is_finite(config->speed_ki)) ||
      (!math_is_finite(config->position_kp)) ||
      (config->speed_kp < 0.0f) ||
      (config->speed_kp > MOTOR_AXIS_GAIN_MAX) ||
      (config->speed_ki < 0.0f) ||
      (config->speed_ki > MOTOR_AXIS_GAIN_MAX) ||
      (config->position_kp < 0.0f) ||
      (config->position_kp > MOTOR_AXIS_GAIN_MAX)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  if ((config->position_speed_limit <= 0) ||
      (config->speed_error_deadband < 0) ||
      (config->position_tolerance < 0) ||
      (config->output_limit <= 0) ||
      (config->output_limit > MOTOR_OUTPUT_MAX)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  return PLATFORM_OK;
}

/**
 * @brief  更新速度 PI 控制器的比例和积分增益。
 * @param  self 已经完成绑定的电机轴对象。
 * @param  speed_kp 新的速度比例增益，范围为 0.0 至 1000.0。
 * @param  speed_ki 新的速度积分增益，范围为 0.0 至 1000.0。
 * @return PLATFORM_OK 表示参数已应用，其他 PLATFORM_* 表示失败。
 * @note   更新成功后清空积分，防止旧参数积累量造成输出突变。
 */
int motor_axis_set_speed_gains(struct motor_axis *self,
                               float speed_kp,
                               float speed_ki)
{
  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((!math_is_finite(speed_kp)) ||
      (!math_is_finite(speed_ki)) ||
      (speed_kp < 0.0f) || (speed_kp > MOTOR_AXIS_GAIN_MAX) ||
      (speed_ki < 0.0f) || (speed_ki > MOTOR_AXIS_GAIN_MAX)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  self->config.speed_kp = speed_kp;
  self->config.speed_ki = speed_ki;
  self->speed_integral = 0.0f;
  return PLATFORM_OK;
}

/**
 * @brief  校验被组合的电机和编码器是否可由组件独占。
 * @param  self 待初始化的电机轴对象。
 * @param  actuator 被组合的电机对象。
 * @param  encoder 被组合的编码器对象。
 * @return PLATFORM_OK 表示有效；其他 PLATFORM_* 表示失败。
 */
static int motor_axis_dependencies_check(
    const struct motor_axis *self,
    const struct motor *actuator,
    const struct encoder *encoder)
{
  if ((self == NULL) || (actuator == NULL) || (encoder == NULL) ||
      (actuator == &self->base)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((actuator->output_ops == NULL) ||
      (actuator->output_ops->set_output == NULL) ||
      (actuator->output_ops->coast == NULL) ||
      (actuator->output_ops->brake == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  if (motor_is_initialized(actuator) ||
      encoder_is_initialized(encoder)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return PLATFORM_OK;
}

/**
 * @brief  启动被组合的电机和编码器并建立初始位置。
 * @param  base 电机轴的 motor 基础对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int motor_axis_start(struct motor *base)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);
  int result;

  result = motor_start(self->actuator);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = encoder_start(self->encoder);
  if (result != PLATFORM_OK) {
    (void)motor_stop(self->actuator);
    return result;
  }
  result = encoder_reset(self->encoder);
  if (result != PLATFORM_OK) {
    (void)encoder_stop(self->encoder);
    (void)motor_stop(self->actuator);
    return result;
  }
  result = low_pass_filter_init(&self->speed_filter,
                                self->config.speed_filter_alpha);
  if (result != PLATFORM_OK) {
    (void)encoder_stop(self->encoder);
    (void)motor_stop(self->actuator);
    return result;
  }

  self->current_position = 0;
  self->previous_position = 0;
  self->current_speed = 0;
  self->sample_valid = false;
  self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
  motor_axis_reset_control(self);
  return PLATFORM_OK;
}

/**
 * @brief  停止被组合的电机和编码器。
 * @param  base 电机轴的 motor 基础对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   允许在部分停止后重试，以便恢复一致的关闭状态。
 */
static int motor_axis_stop(struct motor *base)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);
  int result;

  if (motor_is_initialized(self->actuator)) {
    result = motor_stop(self->actuator);
    if (result != PLATFORM_OK) {
      return result;
    }
  }
  if (encoder_is_initialized(self->encoder)) {
    result = encoder_stop(self->encoder);
    if (result != PLATFORM_OK) {
      return result;
    }
  }

  self->current_speed = 0;
  self->sample_valid = false;
  self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
  motor_axis_reset_control(self);
  return PLATFORM_OK;
}

/**
 * @brief  切换到开环模式并直接设置底层电机输出。
 * @param  base 电机轴的 motor 基础对象。
 * @param  output 范围为 -1000 至 1000 的开环输出。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int motor_axis_set_output(struct motor *base, int16_t output)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);
  int result;

  result = motor_set_output(self->actuator, output);
  if (result == PLATFORM_OK) {
    self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
    motor_axis_reset_control(self);
  }
  return result;
}

/**
 * @brief  切换到开环模式并让底层电机滑行。
 * @param  base 电机轴的 motor 基础对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int motor_axis_coast(struct motor *base)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);
  int result;

  result = motor_coast(self->actuator);
  if (result == PLATFORM_OK) {
    self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
    motor_axis_reset_control(self);
  }
  return result;
}

/**
 * @brief  切换到开环模式并让底层电机制动。
 * @param  base 电机轴的 motor 基础对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int motor_axis_brake(struct motor *base)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);
  int result;

  result = motor_brake(self->actuator);
  if (result == PLATFORM_OK) {
    self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
    motor_axis_reset_control(self);
  }
  return result;
}

/**
 * @brief  设置速度模式的目标速度。
 * @param  base 电机轴的 motor 基础对象。
 * @param  speed 目标速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示目标已保存。
 */
static int motor_axis_set_speed(struct motor *base, int32_t speed)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);

  /* 仅在切入速度模式时清积分，连续调速时保留 PI 控制状态。 */
  if (self->control_mode != MOTOR_AXIS_MODE_SPEED) {
    self->speed_integral = 0.0f;
  }
  self->target_speed = speed;
  self->control_mode = MOTOR_AXIS_MODE_SPEED;
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次更新计算出的实际速度。
 * @param  base 电机轴的 motor 基础对象。
 * @param  speed 用于接收编码器计数每秒的变量。
 * @return PLATFORM_OK 表示读取成功。
 */
static int motor_axis_get_speed(struct motor *base, int32_t *speed)
{
  const struct motor_axis *self =
      container_of(base, struct motor_axis, base);

  *speed = self->current_speed;
  return PLATFORM_OK;
}

/**
 * @brief  设置位置模式的目标累计计数。
 * @param  base 电机轴的 motor 基础对象。
 * @param  position 目标位置，单位为编码器累计计数。
 * @return PLATFORM_OK 表示目标已保存。
 */
static int motor_axis_set_position(struct motor *base,
                                   int32_t position)
{
  struct motor_axis *self =
      container_of(base, struct motor_axis, base);

  self->target_position = position;
  self->speed_integral = 0.0f;
  self->control_mode = MOTOR_AXIS_MODE_POSITION;
  return PLATFORM_OK;
}

/**
 * @brief  获取最近一次编码器采样得到的当前位置。
 * @param  base 电机轴的 motor 基础对象。
 * @param  position 用于接收编码器累计计数的变量。
 * @return PLATFORM_OK 表示读取成功。
 */
static int motor_axis_get_position(struct motor *base,
                                  int32_t *position)
{
  const struct motor_axis *self =
      container_of(base, struct motor_axis, base);

  *position = self->current_position;
  return PLATFORM_OK;
}

/* 电机轴生命周期操作表。 */
static const struct motor_ops s_ops = {
  .start = motor_axis_start,
  .stop = motor_axis_stop,
};

/* 电机轴开环输出能力表。 */
static const struct motor_output_ops s_output_ops = {
  .set_output = motor_axis_set_output,
  .coast = motor_axis_coast,
  .brake = motor_axis_brake,
};

/* 电机轴速度能力表。 */
static const struct motor_speed_ops s_speed_ops = {
  .set_speed = motor_axis_set_speed,
  .get_speed = motor_axis_get_speed,
};

/* 电机轴位置能力表。 */
static const struct motor_position_ops s_position_ops = {
  .set_position = motor_axis_set_position,
  .get_position = motor_axis_get_position,
};

/**
 * @brief  根据本周期编码器位置计算实际速度。
 * @param  self 已启动的电机轴对象。
 * @param  position 本周期读取的编码器累计计数。
 * @return 计算得到的编码器计数每秒。
 */
static int32_t motor_axis_calculate_speed(
    const struct motor_axis *self,
    int32_t position)
{
  int64_t delta;
  int64_t scaled_speed;

  delta = (int64_t)position -
          (int64_t)self->previous_position;
  scaled_speed =
      (delta * MOTOR_AXIS_COUNTS_PER_SECOND_SCALE) /
      (int64_t)self->config.update_period_ms;
  return motor_axis_saturate_int32(scaled_speed);
}

/**
 * @brief  根据位置误差计算速度内环目标。
 * @param  self 已启动的电机轴对象。
 * @return 经过位置容差和速度限幅处理的目标速度。
 */
static int32_t motor_axis_position_speed_target(
    const struct motor_axis *self)
{
  int64_t error;
  float speed;
  float limit;

  error = (int64_t)self->target_position -
          (int64_t)self->current_position;
  if ((error >= -(int64_t)self->config.position_tolerance) &&
      (error <= (int64_t)self->config.position_tolerance)) {
    return 0;
  }

  limit = (float)self->config.position_speed_limit;
  speed = math_clamp_float(self->config.position_kp * (float)error,
                           -limit,
                           limit);
  return motor_axis_saturate_int32((int64_t)speed);
}

/**
 * @brief  执行速度 PI 计算并应用抗积分饱和。
 * @param  self 已启动的电机轴对象。
 * @param  target_speed 本周期目标速度，单位为编码器计数每秒。
 * @return 限幅并四舍五入后的电机输出千分比。
 */
static int16_t motor_axis_speed_control(
    struct motor_axis *self,
    int32_t target_speed)
{
  int64_t error_count;
  float error;
  float candidate_integral;
  float raw_output;
  float limited_output;
  float period_s;
  float output_limit;

  error_count = (int64_t)target_speed -
                (int64_t)self->current_speed;
  period_s = (float)self->config.update_period_ms /
             MOTOR_AXIS_MS_PER_SECOND;

  /* 死区内关闭比例项并暂停积分，保留已有积分输出。 */
  if ((error_count >= -(int64_t)self->config.speed_error_deadband) &&
      (error_count <= (int64_t)self->config.speed_error_deadband)) {
    error = 0.0f;
    candidate_integral = self->speed_integral;
  } else {
    error = (float)error_count;
    candidate_integral =
        self->speed_integral + (error * period_s);
  }
  raw_output = (self->config.speed_kp * error) +
               (self->config.speed_ki * candidate_integral);
  output_limit = (float)self->config.output_limit;
  limited_output = math_clamp_float(raw_output,
                                    -output_limit,
                                    output_limit);

  /* 饱和时只允许能够让输出离开限幅方向的误差继续积分。 */
  if ((raw_output == limited_output) ||
      ((raw_output > output_limit) && (error < 0.0f)) ||
      ((raw_output < -output_limit) && (error > 0.0f))) {
    self->speed_integral = candidate_integral;
  } else {
    raw_output =
        (self->config.speed_kp * error) +
        (self->config.speed_ki * self->speed_integral);
    limited_output = math_clamp_float(raw_output,
                                      -output_limit,
                                      output_limit);
  }

  return motor_axis_round_output(limited_output);
}

/**
 * @brief  初始化闭环电机轴并绑定 motor 基础对象。
 * @param  self 调用者提供的电机轴对象。
 * @param  name 逻辑名称，生命周期必须长于 self。
 * @param  actuator 支持输出、滑行和制动的电机对象。
 * @param  encoder 与 actuator 机械连接的增量编码器对象。
 * @param  config 控制周期、增益和限幅配置。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int motor_axis_init(struct motor_axis *self,
                    const char *name,
                    struct motor *actuator,
                    struct encoder *encoder,
                    const struct motor_axis_config *config)
{
  int result;

  if ((self == NULL) || (name == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  result = motor_axis_dependencies_check(self, actuator, encoder);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = motor_axis_config_check(config);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = low_pass_filter_init(&self->speed_filter,
                                config->speed_filter_alpha);
  if (result != PLATFORM_OK) {
    return result;
  }

  result = motor_bind(&self->base, name, &s_ops, &s_output_ops,
                      &s_speed_ops, &s_position_ops);
  if (result != PLATFORM_OK) {
    return result;
  }

  self->actuator = actuator;
  self->encoder = encoder;
  self->config = *config;
  self->current_speed = 0;
  self->current_position = 0;
  self->previous_position = 0;
  self->sample_valid = false;
  self->control_mode = MOTOR_AXIS_MODE_OPEN_LOOP;
  motor_axis_reset_control(self);
  return PLATFORM_OK;
}

/**
 * @brief  执行一次编码器采样和闭环控制计算。
 * @param  self 已通过 motor_start() 启动的电机轴对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示采样或输出失败。
 */
int motor_axis_update(struct motor_axis *self)
{
  int32_t position;
  int32_t raw_speed;
  int32_t target_speed;
  int16_t output;
  float filtered_speed;
  int result;

  if (self == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!motor_is_initialized(&self->base) ||
      (self->actuator == NULL) || (self->encoder == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = encoder_read_count(self->encoder, &position);
  if (result != PLATFORM_OK) {
    (void)motor_coast(self->actuator);
    return result;
  }

  self->current_position = position;
  if (!self->sample_valid) {
    self->previous_position = position;
    self->current_speed = 0;
    self->sample_valid = true;
    return PLATFORM_OK;
  }
  raw_speed = motor_axis_calculate_speed(self, position);
  result = low_pass_filter_update(&self->speed_filter,
                                  (float)raw_speed,
                                  &filtered_speed);
  if (result != PLATFORM_OK) {
    (void)motor_coast(self->actuator);
    return result;
  }
  self->current_speed = motor_axis_saturate_int32(
      (int64_t)((filtered_speed >= 0.0f) ?
                (filtered_speed + 0.5f) :
                (filtered_speed - 0.5f)));
  self->previous_position = position;

  if (self->control_mode == MOTOR_AXIS_MODE_OPEN_LOOP) {
    return PLATFORM_OK;
  }
  target_speed = (self->control_mode == MOTOR_AXIS_MODE_POSITION) ?
                 motor_axis_position_speed_target(self) :
                 self->target_speed;
  output = motor_axis_speed_control(self, target_speed);
  result = motor_set_output(self->actuator, output);
  if (result != PLATFORM_OK) {
    (void)motor_coast(self->actuator);
  }
  return result;
}

/**
 * @brief  获取电机轴对应的 motor 基础对象。
 * @param  self 电机轴对象。
 * @return motor 基础对象；self 为 NULL 时返回 NULL。
 */
struct motor *motor_axis_get_motor(struct motor_axis *self)
{
  return (self != NULL) ? &self->base : NULL;
}
