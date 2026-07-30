/* SPDX-License-Identifier: MIT */
/**
 * @file    at8236_motor.c
 * @brief   基于两路Platform PWM的AT8236慢衰减电机驱动。
 * @author  weixin
 *
 * 驱动采用AT8236资料定义的慢衰减方式：一个输入保持高电平，另一个
 * 输入输出PWM。零输出明确映射为IN1=0、IN2=0的滑行状态。
 */
#include "at8236_motor.h"

#include "container_of.h"
#include "math_utils.h"
#include "platform/inferface/pwm/pwm.h"
#include "result.h"

#include <stddef.h>
#include <stdint.h>

#define AT8236_OUTPUT_SCALE UINT32_C(1000)
#define AT8236_OUTPUT_ROUND UINT32_C(500)

/**
 * @brief  尝试把两个AT8236输入恢复为滑行状态。
 * @param  self AT8236具体对象。
 * @return 首个PWM写入错误；全部成功时返回PLATFORM_OK。
 */
static int at8236_force_coast(const struct at8236_motor *self)
{
  int first_result;
  int second_result;

  first_result = pwm_set_duty(self->in1_channel,
                              PLATFORM_PWM_DUTY_MIN);
  second_result = pwm_set_duty(self->in2_channel,
                               PLATFORM_PWM_DUTY_MIN);
  return (first_result != PLATFORM_OK) ?
         first_result : second_result;
}

/**
 * @brief  写入一组AT8236输入占空比并在失败时回到滑行。
 * @param  self AT8236具体对象。
 * @param  in1_duty IN1占空比。
 * @param  in2_duty IN2占空比。
 * @param  in2_first true表示先更新IN2，防止反转时出现正向毛刺。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_write_pair(const struct at8236_motor *self,
                             uint16_t in1_duty,
                             uint16_t in2_duty,
                             bool in2_first)
{
  int result;

  if (in2_first) {
    result = pwm_set_duty(self->in2_channel, in2_duty);
    if (result == PLATFORM_OK) {
      result = pwm_set_duty(self->in1_channel, in1_duty);
    }
  } else {
    result = pwm_set_duty(self->in1_channel, in1_duty);
    if (result == PLATFORM_OK) {
      result = pwm_set_duty(self->in2_channel, in2_duty);
    }
  }
  if (result != PLATFORM_OK) {
    (void)at8236_force_coast(self);
  }

  return result;
}

/**
 * @brief  将输出千分比换算为有效驱动占空比。
 * @param  output 范围-1000至1000的电机输出。
 * @return 输出幅值对应的0至10000万分比占空比。
 */
static uint16_t at8236_output_to_duty(int16_t output)
{
  uint32_t magnitude;

  magnitude = math_abs_i32((int32_t)output);
  return (uint16_t)((magnitude *
                    (uint32_t)PLATFORM_PWM_DUTY_MAX +
                    AT8236_OUTPUT_ROUND) /
                   AT8236_OUTPUT_SCALE);
}

/**
 * @brief  启动AT8236两路PWM并保持滑行。
 * @param  base Motor基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_start(struct motor *base)
{
  struct at8236_motor *self =
      container_of(base, struct at8236_motor, base);
  int result;

  result = at8236_force_coast(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = pwm_enable(self->in1_channel);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = pwm_enable(self->in2_channel);
  if (result != PLATFORM_OK) {
    (void)pwm_disable(self->in1_channel);
    return result;
  }

  self->current_output = MOTOR_OUTPUT_STOP;
  return PLATFORM_OK;
}

/**
 * @brief  令AT8236滑行并停止两路PWM。
 * @param  base Motor基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_stop(struct motor *base)
{
  struct at8236_motor *self =
      container_of(base, struct at8236_motor, base);
  int result;
  int current;

  result = at8236_force_coast(self);
  current = pwm_disable(self->in1_channel);
  if (result == PLATFORM_OK) {
    result = current;
  }
  current = pwm_disable(self->in2_channel);
  if (result == PLATFORM_OK) {
    result = current;
  }
  if (result == PLATFORM_OK) {
    self->current_output = MOTOR_OUTPUT_STOP;
  }
  return result;
}

/**
 * @brief  按慢衰减模式设置AT8236方向和输出。
 * @param  base Motor基础对象。
 * @param  output 范围-1000至1000的有符号输出千分比。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_set_output(struct motor *base, int16_t output)
{
  struct at8236_motor *self =
      container_of(base, struct at8236_motor, base);
  uint16_t drive_duty;
  uint16_t decay_duty;
  int result;

  if (self->reversed) {
    output = (int16_t)(-output);
  }
  if (output == MOTOR_OUTPUT_STOP) {
    result = at8236_force_coast(self);
  } else {
    if (((self->current_output < 0) && (output > 0)) ||
        ((self->current_output > 0) && (output < 0))) {
      result = at8236_force_coast(self);
      if (result != PLATFORM_OK) {
        return result;
      }
    }
    drive_duty = at8236_output_to_duty(output);
    decay_duty = PLATFORM_PWM_DUTY_MAX - drive_duty;
    result = (output > 0) ?
             at8236_write_pair(self,
                               PLATFORM_PWM_DUTY_MAX,
                               decay_duty,
                               false) :
             at8236_write_pair(self,
                               decay_duty,
                               PLATFORM_PWM_DUTY_MAX,
                               true);
  }
  if (result == PLATFORM_OK) {
    self->current_output = output;
  }
  return result;
}

/**
 * @brief  设置AT8236为IN1=0、IN2=0的滑行状态。
 * @param  base Motor基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_coast(struct motor *base)
{
  struct at8236_motor *self =
      container_of(base, struct at8236_motor, base);
  int result;

  result = at8236_force_coast(self);
  if (result == PLATFORM_OK) {
    self->current_output = MOTOR_OUTPUT_STOP;
  }
  return result;
}

/**
 * @brief  设置AT8236为IN1=1、IN2=1的刹车状态。
 * @param  base Motor基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int at8236_brake(struct motor *base)
{
  struct at8236_motor *self =
      container_of(base, struct at8236_motor, base);
  int result;

  result = at8236_write_pair(self,
                             PLATFORM_PWM_DUTY_MAX,
                             PLATFORM_PWM_DUTY_MAX,
                             false);
  if (result == PLATFORM_OK) {
    self->current_output = MOTOR_OUTPUT_STOP;
  }
  return result;
}

static const struct motor_ops s_ops = {
  .start = at8236_start,
  .stop = at8236_stop,
};

static const struct motor_output_ops s_output_ops = {
  .set_output = at8236_set_output,
  .coast = at8236_coast,
  .brake = at8236_brake,
};

/**
 * @brief  初始化AT8236具体对象并绑定Motor基础类。
 * @param  self 调用者持有的AT8236对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  in1_channel AT8236 IN1连接的Platform PWM通道。
 * @param  in2_channel AT8236 IN2连接的Platform PWM通道。
 * @param  reversed true表示反转正负输出定义。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int at8236_motor_init(struct at8236_motor *self,
                      const char *name,
                      int32_t in1_channel,
                      int32_t in2_channel,
                      bool reversed)
{
  int result;

  if ((self == NULL) || (name == NULL) ||
      (in1_channel < 0) || (in2_channel < 0) ||
      (in1_channel == in2_channel)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = motor_bind(&self->base,
                      name,
                      &s_ops,
                      &s_output_ops,
                      NULL,
                      NULL);
  if (result != PLATFORM_OK) {
    return result;
  }
  self->in1_channel = in1_channel;
  self->in2_channel = in2_channel;
  self->current_output = MOTOR_OUTPUT_STOP;
  self->reversed = reversed;
  return PLATFORM_OK;
}

/**
 * @brief  获取AT8236对应的Motor基础对象。
 * @param  self AT8236具体对象。
 * @return Motor基础对象；self为NULL时返回NULL。
 */
struct motor *at8236_motor_get_motor(struct at8236_motor *self)
{
  return (self != NULL) ? &self->base : NULL;
}
