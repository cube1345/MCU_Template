/* SPDX-License-Identifier: MIT */
/**
 * @file    board_drive.c
 * @brief   STM32F407双路AT8236电机与编码器板级资源绑定。
 * @author  weixin
 *
 * AT8236接线为AIN1=PA5、AIN2=PA1、BIN1=PA2、BIN2=PA3，四路PWM
 * 均由TIM2产生。左编码器使用TIM4的
 * PD12/PD13，右编码器使用TIM3的PA6/PA7。右侧电机与编码器统一
 * 反向，使应用层正值始终表达整车向前。
 */
#include "board_drive.h"

#include "at8236_motor.h"
#include "motor_axis.h"
#include "quadrature_encoder.h"
#include "result.h"
#include "stm32f4.h"
#include "stm32f4_encoder_counter.h"

#include <stdbool.h>
#include <stddef.h>

/* 两路编码器输入捕获共用的数字滤波等级。 */
#define BOARD_ENCODER_INPUT_FILTER UINT32_C(6)
/* 速度误差进入该范围后关闭比例项并暂停积分，单位为 count/s。 */
#define BOARD_DRIVE_SPEED_ERROR_DEADBAND_COUNT_S INT32_C(300)

/*
 * 当前机械系统尚未提供编码器每圈计数，因此速度暂以 count/s 表达。
 * 输出上限为 800，将闭环最大驱动力限制为电机输出满量程的 80%。
 */
static const struct motor_axis_config s_axis_config = {
  .update_period_ms = BOARD_DRIVE_UPDATE_PERIOD_MS,
  /* 200 Hz 采样时约等效于 10 Hz 截止频率的一阶低通。 */
  .speed_filter_alpha = 0.2391f,
  .speed_kp = BOARD_DRIVE_SPEED_KP_DEFAULT,
  .speed_ki = BOARD_DRIVE_SPEED_KI_DEFAULT,
  .speed_error_deadband = BOARD_DRIVE_SPEED_ERROR_DEADBAND_COUNT_S,
  .position_kp = 2.0f,
  .position_speed_limit = 1000,
  .position_tolerance = 2,
  .output_limit = 800,
};

/* 左编码器的定时器、引脚和输入滤波板级配置。 */
static const struct stm32f4_encoder_counter_config
    s_left_counter_config = {
  .id = STM32F4_ENCODER_TIM4,
  .name = "left_encoder_counter",
  .pin_a = STM32F4_PIN_PD12,
  .pin_b = STM32F4_PIN_PD13,
  .input_filter = BOARD_ENCODER_INPUT_FILTER,
};

/* 右编码器的定时器、引脚和输入滤波板级配置。 */
static const struct stm32f4_encoder_counter_config
    s_right_counter_config = {
  .id = STM32F4_ENCODER_TIM3,
  .name = "right_encoder_counter",
  .pin_a = STM32F4_PIN_PA6,
  .pin_b = STM32F4_PIN_PA7,
  .input_filter = BOARD_ENCODER_INPUT_FILTER,
};

/* 左侧 AT8236 电机具体对象，由 board 层持有静态存储。 */
static struct at8236_motor s_left_motor;
/* 右侧 AT8236 电机具体对象，由 board 层持有静态存储。 */
static struct at8236_motor s_right_motor;
/* 左侧正交编码器具体对象，由 board 层持有静态存储。 */
static struct quadrature_encoder s_left_encoder;
/* 右侧正交编码器具体对象，由 board 层持有静态存储。 */
static struct quadrature_encoder s_right_encoder;
/* 左侧电机与编码器组成的闭环电机轴。 */
static struct motor_axis s_left_axis;
/* 右侧电机与编码器组成的闭环电机轴。 */
static struct motor_axis s_right_axis;
/* 左编码器绑定的 Platform 计数器基础对象。 */
static struct encoder_counter *s_left_counter;
/* 右编码器绑定的 Platform 计数器基础对象。 */
static struct encoder_counter *s_right_counter;
/* 左编码器对象是否已经完成软件绑定。 */
static bool s_left_encoder_bound;
/* 右编码器对象是否已经完成软件绑定。 */
static bool s_right_encoder_bound;
/* 左电机对象是否已经完成软件绑定。 */
static bool s_left_motor_bound;
/* 右电机对象是否已经完成软件绑定。 */
static bool s_right_motor_bound;
/* 左侧闭环电机轴是否已经完成软件绑定。 */
static bool s_left_axis_bound;
/* 右侧闭环电机轴是否已经完成软件绑定。 */
static bool s_right_axis_bound;
/* 电机、编码器和闭环轴对象是否已经全部完成软件绑定。 */
static bool s_objects_bound;

/**
 * @brief  创建 Platform 计数器并绑定电机、编码器和闭环轴对象。
 * @return PLATFORM_OK 表示成功或已经绑定，其他 PLATFORM_* 表示失败。
 * @note   本函数会准备 MCU 公共子系统，但不启动 PWM 或编码器计数。
 *         只能在系统启动阶段调用，不支持多个任务并发调用。
 */
int board_drive_bind(void)
{
  int result;

  if (s_objects_bound) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  if (result != PLATFORM_OK) {
    return result;
  }

  s_left_counter =
      stm32f4_encoder_counter_create(&s_left_counter_config);
  s_right_counter =
      stm32f4_encoder_counter_create(&s_right_counter_config);
  if ((s_left_counter == NULL) || (s_right_counter == NULL)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  if (!s_left_encoder_bound) {
    result = quadrature_encoder_init(&s_left_encoder,
                                     "left_encoder",
                                     s_left_counter,
                                     false);
    s_left_encoder_bound = (result == PLATFORM_OK);
  }
  if ((result == PLATFORM_OK) && !s_right_encoder_bound) {
    result = quadrature_encoder_init(&s_right_encoder,
                                     "right_encoder",
                                     s_right_counter,
                                     true);
    s_right_encoder_bound = (result == PLATFORM_OK);
  }
  if ((result == PLATFORM_OK) && !s_left_motor_bound) {
    result = at8236_motor_init(&s_left_motor,
                               "left_motor",
                               STM32F4_PWM_TIM2_CH1,
                               STM32F4_PWM_TIM2_CH2,
                               false);
    s_left_motor_bound = (result == PLATFORM_OK);
  }
  if ((result == PLATFORM_OK) && !s_right_motor_bound) {
    result = at8236_motor_init(&s_right_motor,
                               "right_motor",
                               STM32F4_PWM_TIM2_CH3,
                               STM32F4_PWM_TIM2_CH4,
                               true);
    s_right_motor_bound = (result == PLATFORM_OK);
  }
  if ((result == PLATFORM_OK) && !s_left_axis_bound) {
    result = motor_axis_init(&s_left_axis,
                             "left_motor_axis",
                             &s_left_motor.base,
                             &s_left_encoder.base,
                             &s_axis_config);
    s_left_axis_bound = (result == PLATFORM_OK);
  }
  if ((result == PLATFORM_OK) && !s_right_axis_bound) {
    result = motor_axis_init(&s_right_axis,
                             "right_motor_axis",
                             &s_right_motor.base,
                             &s_right_encoder.base,
                             &s_axis_config);
    s_right_axis_bound = (result == PLATFORM_OK);
  }
  if (result == PLATFORM_OK) {
    s_objects_bound = true;
  }
  return result;
}

/**
 * @brief  启动左右两个闭环电机轴及其底层资源。
 * @return PLATFORM_OK 表示成功或已经启动，其他 PLATFORM_* 表示失败。
 * @note   任一侧启动失败时会停止本次已经启动的另一侧，避免单侧运行。
 */
int board_drive_init(void)
{
  bool left_was_initialized;
  int result;

  result = board_drive_bind();
  if (result != PLATFORM_OK) {
    return result;
  }

  left_was_initialized = motor_is_initialized(&s_left_axis.base);
  result = motor_start(&s_left_axis.base);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = motor_start(&s_right_axis.base);
  if ((result != PLATFORM_OK) && !left_was_initialized) {
    (void)motor_stop(&s_left_axis.base);
  }
  return result;
}

/**
 * @brief  执行一次左右电机轴的编码器采样和速度闭环计算。
 * @return PLATFORM_OK 表示两侧更新成功，其他 PLATFORM_* 表示失败。
 * @note   必须由单一任务每 5 ms 调用；任一侧失败时两侧都会滑行。
 */
int board_drive_update(void)
{
  int left_result;
  int right_result;

  left_result = motor_axis_update(&s_left_axis);
  right_result = motor_axis_update(&s_right_axis);
  if ((left_result != PLATFORM_OK) ||
      (right_result != PLATFORM_OK)) {
    (void)motor_coast(&s_left_axis.base);
    (void)motor_coast(&s_right_axis.base);
    return (left_result != PLATFORM_OK) ?
           left_result : right_result;
  }

  return PLATFORM_OK;
}

/**
 * @brief  同时更新左右闭环电机轴的速度 PI 参数。
 * @param  speed_kp 比例增益，范围由 motor_axis 组件约束。
 * @param  speed_ki 积分增益，范围由 motor_axis 组件约束。
 * @return PLATFORM_OK 表示两侧参数均已应用，其他 PLATFORM_* 表示失败。
 * @note   两侧使用同一组参数，更新时会分别清空速度积分状态。
 */
int board_drive_set_speed_gains(float speed_kp, float speed_ki)
{
  int result;

  result = motor_axis_set_speed_gains(&s_left_axis,
                                      speed_kp,
                                      speed_ki);
  if (result != PLATFORM_OK) {
    return result;
  }

  return motor_axis_set_speed_gains(&s_right_axis,
                                    speed_kp,
                                    speed_ki);
}

/**
 * @brief  获取左侧电机基础对象。
 * @return 绑定成功后返回 Motor 对象，否则返回 NULL。
 */
struct motor *board_motor_left_get(void)
{
  return s_left_axis_bound ? &s_left_axis.base : NULL;
}

/**
 * @brief  获取右侧电机基础对象。
 * @return 绑定成功后返回 Motor 对象，否则返回 NULL。
 */
struct motor *board_motor_right_get(void)
{
  return s_right_axis_bound ? &s_right_axis.base : NULL;
}

/**
 * @brief  获取左侧编码器基础对象。
 * @return 绑定成功后返回 Encoder 对象，否则返回 NULL。
 */
struct encoder *board_encoder_left_get(void)
{
  return s_left_encoder_bound ? &s_left_encoder.base : NULL;
}

/**
 * @brief  获取右侧编码器基础对象。
 * @return 绑定成功后返回 Encoder 对象，否则返回 NULL。
 */
struct encoder *board_encoder_right_get(void)
{
  return s_right_encoder_bound ? &s_right_encoder.base : NULL;
}
