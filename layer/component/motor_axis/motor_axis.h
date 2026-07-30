/* SPDX-License-Identifier: MIT */
/**
 * @file    motor_axis.h
 * @brief   直流电机与增量编码器组成的闭环电机轴组件。
 * @author  weixin
 *
 * motor_axis 将一个支持开环输出的 motor 与一个 encoder 组合为新的
 * motor 对象。组合后的对象同时支持开环输出、速度和位置能力，应用层
 * 无需了解 AT8236、PWM 或定时器编码器等具体实现。
 */
#ifndef MOTOR_AXIS_H
#define MOTOR_AXIS_H

#include "device/encoder/encoder.h"
#include "device/motor/motor.h"
#include "low_pass_filter.h"

#include <stdbool.h>
#include <stdint.h>

struct motor_axis_config {
  /* motor_axis_update() 的固定调用周期，单位为毫秒。 */
  uint32_t update_period_ms;
  /* 速度低通滤波的新样本权重，范围为 0.0 至 1.0。 */
  float speed_filter_alpha;
  /* 速度环比例增益，输出单位为千分比。 */
  float speed_kp;
  /* 速度环积分增益，输出单位为千分比。 */
  float speed_ki;
  /* 速度误差死区，单位为编码器计数每秒。 */
  int32_t speed_error_deadband;
  /* 位置环比例增益，输出单位为编码器计数每秒。 */
  float position_kp;
  /* 位置模式允许的最大目标速度，单位为编码器计数每秒。 */
  int32_t position_speed_limit;
  /* 到位判断容差，单位为编码器计数。 */
  int32_t position_tolerance;
  /* 闭环允许输出的最大绝对值，范围为 1 至 1000。 */
  int16_t output_limit;
};

struct motor_axis {
  /* 向应用层暴露的 motor 基础对象，必须保持为首成员。 */
  struct motor base;
  /* 被组合的电机对象；组件只保存引用并独占其运行生命周期。 */
  struct motor *actuator;
  /* 被组合的编码器对象；组件只保存引用并独占其运行生命周期。 */
  struct encoder *encoder;
  /* 控制周期、闭环增益和安全限幅配置。 */
  struct motor_axis_config config;
  /* 速度模式的目标速度，单位为编码器计数每秒。 */
  int32_t target_speed;
  /* 位置模式的目标位置，单位为编码器累计计数。 */
  int32_t target_position;
  /* 最近一次更新计算出的速度，单位为编码器计数每秒。 */
  int32_t current_speed;
  /* 最近一次更新读取的位置，单位为编码器累计计数。 */
  int32_t current_position;
  /* 上一次更新的位置，用于计算本周期速度。 */
  int32_t previous_position;
  /* 速度 PI 控制器的积分状态。 */
  float speed_integral;
  /* 编码器差分速度使用的一阶低通滤波器。 */
  struct low_pass_filter speed_filter;
  /* 当前控制模式，仅由本组件内部维护。 */
  uint8_t control_mode;
  /* 首次编码器样本建立后置 true。 */
  bool sample_valid;
};

/**
 * @brief  初始化闭环电机轴并绑定 motor 基础对象。
 * @param  self 调用者提供的电机轴对象。
 * @param  name 逻辑名称，生命周期必须长于 self。
 * @param  actuator 支持输出、滑行和制动的电机对象。
 * @param  encoder 与 actuator 机械连接的增量编码器对象。
 * @param  config 控制周期、增益和限幅配置。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数不启动硬件或分配堆内存。两个依赖必须尚未启动，并且
 *         在 motor_axis 运行期间不能被其他模块直接控制。
 */
int motor_axis_init(struct motor_axis *self,
                    const char *name,
                    struct motor *actuator,
                    struct encoder *encoder,
                    const struct motor_axis_config *config);

/**
 * @brief  更新速度 PI 控制器的比例和积分增益。
 * @param  self 已经完成绑定的电机轴对象。
 * @param  speed_kp 新的速度比例增益，范围为 0.0 至 1000.0。
 * @param  speed_ki 新的速度积分增益，范围为 0.0 至 1000.0。
 * @return PLATFORM_OK 表示参数已应用，其他 PLATFORM_* 表示失败。
 * @note   必须由拥有该电机轴的控制任务调用；更新成功后会清空积分状态。
 */
int motor_axis_set_speed_gains(struct motor_axis *self,
                               float speed_kp,
                               float speed_ki);

/**
 * @brief  执行一次编码器采样和闭环控制计算。
 * @param  self 已通过 motor_start() 启动的电机轴对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示采样或输出失败。
 * @note   必须按 config.update_period_ms 固定周期从单一任务调用，
 *         不能在中断或多个任务中并发调用。
 */
int motor_axis_update(struct motor_axis *self);

/**
 * @brief  获取电机轴对应的 motor 基础对象。
 * @param  self 电机轴对象。
 * @return motor 基础对象；self 为 NULL 时返回 NULL。
 */
struct motor *motor_axis_get_motor(struct motor_axis *self);

#endif /* MOTOR_AXIS_H */
