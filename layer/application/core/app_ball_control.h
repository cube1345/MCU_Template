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

/* 相对水管中心的小球目标位置范围，单位为像素。 */
#define BALL_CONTROL_TARGET_X_MIN INT16_C(-320)
#define BALL_CONTROL_TARGET_X_MAX INT16_C(320)
#define BALL_CONTROL_OPEN_LOOP_POSITION_DEG_MIN (-40.0F)
#define BALL_CONTROL_OPEN_LOOP_POSITION_DEG_MAX (40.0F)

/* 小球位置环上电默认增益，同时作为WEIXIN参数默认值。 */
#define BALL_CONTROL_POSITION_KP_DEFAULT (0.0F)
#define BALL_CONTROL_POSITION_KI_DEFAULT (0.03F)
#define BALL_CONTROL_POSITION_KD_DEFAULT (0.0F)
/* STM32估计速度到EMM反向制动角度的默认比例，单位为度/(像素/秒)。 */
#define BALL_CONTROL_VELOCITY_DAMPING_DEFAULT (0.05F)

struct ball_control_status {
  int16_t target_x;
  int16_t measured_x;
  /* 视觉位置经过一阶低通后的结果，单位为像素。 */
  float filtered_x;
  /* Jetson帧中的原始小球速度，单位为像素每帧。 */
  int8_t measured_velocity;
  /* STM32根据滤波位置估计并低通处理的速度，单位为像素每秒。 */
  float filtered_velocity;
  int8_t measured_acceleration;
  uint32_t vision_sequence;
  /* 位置PID在叠加速度阻尼前的角度输出，单位为度。 */
  float position_pid_output_deg;
  /* 根据滤波速度计算的反向制动角度，单位为度。 */
  float velocity_damping_deg;
  /* 位置PID减去速度阻尼后的EMM绝对位置目标，单位为度。 */
  float output_position_deg;
  /* 位置和速度同时进入容差范围后置位。 */
  bool arrived;
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
 * @brief  启用视觉监测下的固定EMM绝对角度开环控制。
 * @param  position_deg 固定绝对角度，范围为-40至40度。
 * @return PLATFORM_OK表示成功，参数非法时返回配置错误。
 * @note   调用后位置PID关闭；ball_control_enabled_set()会退出开环模式。
 */
int ball_control_open_loop_position_deg_set(float position_deg);

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
 * @note   位置PID输出减去显式速度阻尼后作为EMM绝对角度目标。
 */
void ball_control_task(void *argument);

#endif /* APP_BALL_CONTROL_H */
