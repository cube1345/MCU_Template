/* SPDX-License-Identifier: MIT */
/**
 * @file    pid.c
 * @brief   离散 PID 控制纯算法实现。
 * @author  weixin
 */
#include "pid.h"

#include "math_utils.h"
#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 PID 配置中所有浮点参数是否为有限值。
 * @param  config：待检查的 PID 配置。
 * @retval true：所有浮点参数均为有限值。
 * @retval false：至少有一个参数为 NaN 或正负无穷大。
 */
static bool pid_config_values_are_finite(const struct pid_config *config)
{
  return math_is_finite(config->kp) &&
         math_is_finite(config->ki) &&
         math_is_finite(config->kd) &&
         math_is_finite(config->output_min) &&
         math_is_finite(config->output_max) &&
         math_is_finite(config->output_deadband) &&
         math_is_finite(config->integral_clear_error) &&
         math_is_finite(config->integral_min) &&
         math_is_finite(config->integral_max);
}

/**
 * @brief  检查 PID 配置的范围和阈值关系。
 * @param  config：待检查的 PID 配置。
 * @retval PLATFORM_OK：配置合法。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：配置内容无效。
 */
static int pid_validate_config(const struct pid_config *config)
{
  if (!pid_config_values_are_finite(config)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  if ((config->output_min > config->output_max) ||
      (config->integral_min > config->integral_max) ||
      (config->output_deadband < 0.0f) ||
      (config->integral_clear_error < 0.0f)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  return PLATFORM_OK;
}

/**
 * @brief  初始化 PID 控制器。
 * @param  self：调用方提供的 PID 对象。
 * @param  config：控制参数，初始化期间会被复制到 self。
 * @retval PLATFORM_OK：初始化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：对象或配置地址无效。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：配置内容无效。
 */
int pid_init(struct pid *self, const struct pid_config *config)
{
  int status;

  if ((self == NULL) || (config == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  status = pid_validate_config(config);
  if (status != PLATFORM_OK) {
    return status;
  }

  self->config = *config;
  self->initialized = true;
  pid_reset(self);
  return PLATFORM_OK;
}

/**
 * @brief  清空 PID 积分和历史误差状态。
 * @param  self：PID 对象；传入 NULL 时不执行操作。
 * @retval None。
 */
void pid_reset(struct pid *self)
{
  if (self == NULL) {
    return;
  }

  self->integral = 0.0f;
  self->previous_error = 0.0f;
  self->has_previous_error = false;
}

/**
 * @brief  根据当前误差更新一次 PID 输出。
 * @param  self：已经初始化的 PID 对象。
 * @param  error：当前误差，通常为目标值减测量值。
 * @param  dt_s：本次更新距离上次更新的时间，单位为秒且必须大于零。
 * @param  output：用于接收限幅后 PID 输出的地址。
 * @retval PLATFORM_OK：计算成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：输入或输出地址无效。
 * @retval PLATFORM_ERR_INVALID_STATE：PID 对象尚未初始化。
 */
int pid_update(struct pid *self,
               float error,
               float dt_s,
               float *output)
{
  float derivative = 0.0f;
  float result;

  if ((self == NULL) || (output == NULL) ||
      (!math_is_finite(error)) || (!math_is_finite(dt_s)) ||
      (dt_s <= 0.0f)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  if ((self->config.integral_clear_error > 0.0f) &&
      (math_abs_float(error) <= self->config.integral_clear_error)) {
    /* 目标附近清空残留积分，防止积分继续推动执行器越过目标。 */
    self->integral = 0.0f;
  } else {
    self->integral += self->config.ki * error * dt_s;
    self->integral = math_clamp_float(self->integral,
                                      self->config.integral_min,
                                      self->config.integral_max);
  }

  if (self->has_previous_error) {
    derivative = (error - self->previous_error) / dt_s;
  }

  result = (self->config.kp * error) + self->integral +
           (self->config.kd * derivative);
  result = math_deadband_float(result,
                               -self->config.output_deadband,
                               self->config.output_deadband);
  result = math_clamp_float(result,
                            self->config.output_min,
                            self->config.output_max);

  if (!math_is_finite(result)) {
    /*
     * 极端增益或误差可能使中间运算溢出。失败时不更新历史误差，
     * 让调用方能够处理错误后重新计算。
     */
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  self->previous_error = error;
  self->has_previous_error = true;
  *output = result;
  return PLATFORM_OK;
}
