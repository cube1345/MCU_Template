/* SPDX-License-Identifier: MIT */
/**
 * @file    motor.c
 * @brief   电机基础对象生命周期与可选能力分派。
 * @author  weixin
 */
#include "motor.h"

#include <stddef.h>

/**
 * @brief  检查电机对象是否已经启动。
 * @param  self 电机对象。
 * @return PLATFORM_OK 表示可用，其他 PLATFORM_* 表示状态错误。
 */
static int motor_ready_check(const struct motor *self)
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
 * @brief  绑定电机基础对象、生命周期表和可选能力表。
 * @param  self 调用者持有的电机对象。
 * @param  name 逻辑名称，生命周期必须长于 self。
 * @param  ops 生命周期操作表，start 和 stop 必须有效。
 * @param  output_ops 输出能力表；不支持输出能力时传入 NULL。
 * @param  speed_ops 速度能力表；不支持速度能力时传入 NULL。
 * @param  position_ops 位置能力表；不支持位置能力时传入 NULL。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int motor_bind(struct motor *self,
               const char *name,
               const struct motor_ops *ops,
               const struct motor_output_ops *output_ops,
               const struct motor_speed_ops *speed_ops,
               const struct motor_position_ops *position_ops)
{
  if ((self == NULL) || (name == NULL) || (ops == NULL) ||
      (ops->start == NULL) || (ops->stop == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((self->ops != NULL) || self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->ops = ops;
  self->output_ops = output_ops;
  self->speed_ops = speed_ops;
  self->position_ops = position_ops;
  self->name = name;
  self->initialized = false;
  return PLATFORM_OK;
}

/**
 * @brief  启动电机设备的底层资源。
 * @param  self 已完成绑定的电机对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int motor_start(struct motor *self)
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
  }
  return result;
}

/**
 * @brief  停止电机设备的底层资源。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int motor_stop(struct motor *self)
{
  int result;

  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = self->ops->stop(self);
  if (result == PLATFORM_OK) {
    self->initialized = false;
  }
  return result;
}

/**
 * @brief  设置开环输出千分比。
 * @param  self 已经启动的电机对象。
 * @param  output 范围 -1000 到 1000；负数反向，0 滑行，正数正向。
 * @return PLATFORM_OK 表示成功；不支持输出能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_set_output(struct motor *self, int16_t output)
{
  int result;

  if ((output < MOTOR_OUTPUT_MIN) ||
      (output > MOTOR_OUTPUT_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->output_ops == NULL) ||
      (self->output_ops->set_output == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->output_ops->set_output(self, output);
}

/**
 * @brief  让电机进入滑行状态。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；不支持输出能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_coast(struct motor *self)
{
  int result;

  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->output_ops == NULL) ||
      (self->output_ops->coast == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->output_ops->coast(self);
}

/**
 * @brief  让电机进入电气制动状态。
 * @param  self 已经启动的电机对象。
 * @return PLATFORM_OK 表示成功；不支持制动能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_brake(struct motor *self)
{
  int result;

  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->output_ops == NULL) ||
      (self->output_ops->brake == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->output_ops->brake(self);
}

/**
 * @brief  设置目标速度。
 * @param  self 已经启动的电机对象。
 * @param  speed 目标速度，单位由具体驱动或组件在自身头文件中定义。
 * @return PLATFORM_OK 表示成功；不支持速度能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_set_speed(struct motor *self, int32_t speed)
{
  int result;

  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->speed_ops == NULL) ||
      (self->speed_ops->set_speed == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->speed_ops->set_speed(self, speed);
}

/**
 * @brief  获取当前速度。
 * @param  self 已经启动的电机对象。
 * @param  speed 用于接收当前速度的变量指针。
 * @return PLATFORM_OK 表示成功；不支持速度能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_get_speed(struct motor *self, int32_t *speed)
{
  int result;

  if (speed == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->speed_ops == NULL) ||
      (self->speed_ops->get_speed == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->speed_ops->get_speed(self, speed);
}

/**
 * @brief  设置目标位置。
 * @param  self 已经启动的电机对象。
 * @param  position 目标位置，单位由具体驱动或组件在自身头文件中定义。
 * @return PLATFORM_OK 表示成功；不支持位置能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_set_position(struct motor *self, int32_t position)
{
  int result;

  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->position_ops == NULL) ||
      (self->position_ops->set_position == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->position_ops->set_position(self, position);
}

/**
 * @brief  获取当前位置。
 * @param  self 已经启动的电机对象。
 * @param  position 用于接收当前位置的变量指针。
 * @return PLATFORM_OK 表示成功；不支持位置能力时返回 PLATFORM_ERR_UNSUPPORTED。
 */
int motor_get_position(struct motor *self, int32_t *position)
{
  int result;

  if (position == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = motor_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((self->position_ops == NULL) ||
      (self->position_ops->get_position == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->position_ops->get_position(self, position);
}

/**
 * @brief  查询电机是否支持开环输出能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_output(const struct motor *self)
{
  return (self != NULL) && (self->output_ops != NULL) &&
         ((self->output_ops->set_output != NULL) ||
          (self->output_ops->coast != NULL) ||
          (self->output_ops->brake != NULL));
}

/**
 * @brief  查询电机是否支持速度能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_speed(const struct motor *self)
{
  return (self != NULL) && (self->speed_ops != NULL) &&
         ((self->speed_ops->set_speed != NULL) ||
          (self->speed_ops->get_speed != NULL));
}

/**
 * @brief  查询电机是否支持位置能力。
 * @param  self 电机对象。
 * @return true 表示支持，false 表示不支持或对象无效。
 */
bool motor_supports_position(const struct motor *self)
{
  return (self != NULL) && (self->position_ops != NULL) &&
         ((self->position_ops->set_position != NULL) ||
          (self->position_ops->get_position != NULL));
}

/**
 * @brief  获取电机逻辑名称。
 * @param  self 电机对象。
 * @return 对象名称；对象无效时返回 "unknown"。
 */
const char *motor_get_name(const struct motor *self)
{
  if ((self == NULL) || (self->name == NULL)) {
    return "unknown";
  }

  return self->name;
}

/**
 * @brief  查询电机底层资源是否已经启动。
 * @param  self 电机对象。
 * @return true 表示已经启动；false 表示未启动或对象无效。
 */
bool motor_is_initialized(const struct motor *self)
{
  return (self != NULL) && self->initialized;
}
