/* SPDX-License-Identifier: MIT */
/**
 * @file    app_ball_control.h
 * @brief   视觉小球位置闭环控制任务接口。
 * @author  weixin
 */
#ifndef APP_BALL_CONTROL_H
#define APP_BALL_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/* 小球位置环上电默认增益，同时作为WEIXIN参数默认值。 */
#define BALL_CONTROL_POSITION_KP_DEFAULT (0.02F)
#define BALL_CONTROL_POSITION_KI_DEFAULT (0.1F)
#define BALL_CONTROL_POSITION_KD_DEFAULT (0.08F)

struct ball_control_status {
  int16_t target_x;
  int16_t measured_x;
  /* 视觉位置经过一阶低通后的结果，单位为像素。 */
  float filtered_x;
  /* Jetson帧中的原始小球速度，单位为像素每帧。 */
  int8_t measured_velocity;
  /* 原始小球速度经过一阶低通后的结果，单位为像素每帧。 */
  float filtered_velocity;
  int8_t measured_acceleration;
  uint32_t vision_sequence;
  /* PID输出及EMM绝对位置目标，单位为度。 */
  float output_position_deg;
  bool enabled;
  bool vision_valid;
};

/**
 * @brief  设置小球位置控制目标。
 * @param  target_x 相对水管中心的目标像素位置，范围为-320至320。
 * @return PLATFORM_OK表示成功，范围非法时返回配置错误。
 */
int ball_control_target_x_set(int16_t target_x);

/**
 * @brief  设置小球位置闭环是否启用。
 * @param  enabled true表示启用PID计算，false表示关闭并复位PID。
 * @return PLATFORM_OK。
 * @note   关闭时会复位PID并向EMM绝对位置邮箱发布0度。
 */
int ball_control_enabled_set(bool enabled);

/**
 * @brief  获取小球位置控制状态快照。
 * @param  status 用于接收状态的结构体指针。
 * @return PLATFORM_OK表示成功，参数为空时返回参数错误。
 */
int ball_control_status_get(struct ball_control_status *status);

/**
 * @brief  运行小球位置闭环控制任务。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None。
 * @note   位置误差经PID换算为绝对角度，通过邮箱由EMM任务串行执行。
 */
void ball_control_task(void *argument);

#endif /* APP_BALL_CONTROL_H */
