/* SPDX-License-Identifier: MIT */
/**
 * @file    app_serial_monitor.h
 * @brief   WEIXIN Serial Monitor UART1 通信任务入口。
 * @author  weixin
 */
#ifndef APP_SERIAL_MONITOR_H
#define APP_SERIAL_MONITOR_H

#include <stdint.h>

struct serial_monitor_task3_open_loop_config {
  float positive_drive_angle_deg;
  uint32_t positive_drive_time_ms;
  float positive_brake_angle_deg;
  uint32_t positive_brake_time_ms;
  float positive_hold_angle_deg;
  uint32_t positive_hold_time_ms;
  float negative_drive_angle_deg;
  uint32_t negative_drive_time_ms;
  float negative_brake_angle_deg;
  uint32_t negative_brake_time_ms;
  float hold_angle_deg;
};

/*
 * 速度环串口调试总开关：1 开启调参和波形，0 关闭并裁剪相关代码。
 * 允许由编译选项预定义该宏覆盖默认值。
 */
#ifndef SERIAL_MONITOR_DRIVE_DEBUG_ENABLE
#define SERIAL_MONITOR_DRIVE_DEBUG_ENABLE 0
#endif

#if ((SERIAL_MONITOR_DRIVE_DEBUG_ENABLE != 0) && \
     (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE != 1))
#error "SERIAL_MONITOR_DRIVE_DEBUG_ENABLE must be 0 or 1"
#endif

/* Jetson报告速度、STM32估计速度及旧PID速度输出波形开关。 */
#ifndef SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE
#define SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE 0
#endif

#if ((SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE != 0) && \
     (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE != 1))
#error "SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE must be 0 or 1"
#endif

/* 小球位置相关波形通道开关；默认只发送位置误差。 */
#ifndef SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE
#define SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE 0
#endif
#ifndef SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE
#define SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE 0
#endif
#ifndef SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE
#define SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE 0
#endif
#ifndef SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE
#define SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE 1
#endif

#if ((SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE != 0) && \
     (SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE != 1))
#error "SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE must be 0 or 1"
#endif
#if ((SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE != 0) && \
     (SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE != 1))
#error "SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE must be 0 or 1"
#endif
#if ((SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE != 0) && \
     (SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE != 1))
#error "SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE must be 0 or 1"
#endif
#if ((SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE != 0) && \
     (SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE != 1))
#error "SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE must be 0 or 1"
#endif

#define SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE \
  ((SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE == 1) || \
   (SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE == 1) || \
   (SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE == 1) || \
   (SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE == 1))

/* 任一波形源开启时，USART1任务按固定周期发送最新波形快照。 */
#define SERIAL_MONITOR_WAVE_ENABLE \
  ((SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1) || \
   (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1) || \
   (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1))

/**
 * @brief  获取上位机请求的电机速度 PI 参数快照。
 * @param  speed_kp 用于接收比例增益的变量。
 * @param  speed_ki 用于接收积分增益的变量。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   本函数只读取待应用值，不直接修改闭环控制器。
 */
int serial_monitor_drive_speed_gains_get(float *speed_kp,
                                         float *speed_ki);

/**
 * @brief  获取WEIXIN上位机设置的小球位置环PID增益快照。
 * @param  position_kp 用于接收位置环比例增益。
 * @param  position_ki 用于接收位置环积分增益。
 * @param  position_kd 用于接收位置环微分增益。
 * @return PLATFORM_OK表示读取成功，参数为空时返回参数错误。
 */
int serial_monitor_ball_position_gains_get(float *position_kp,
                                           float *position_ki,
                                           float *position_kd);

/**
 * @brief  获取WEIXIN上位机设置的小球目标位置。
 * @param  target_x 用于接收目标位置，单位为像素。
 * @return PLATFORM_OK表示读取成功，参数为空时返回参数错误。
 */
int serial_monitor_ball_target_x_get(int16_t *target_x);

/**
 * @brief  更新WEIXIN参数表中的小球目标位置。
 * @param  target_x 相对水管中心的目标位置，单位为像素。
 * @return PLATFORM_OK表示成功，超出范围时返回配置错误。
 */
int serial_monitor_ball_target_x_set(int16_t target_x);

/**
 * @brief  获取小球速度阻尼系数。
 * @param  velocity_damping 用于接收速度到反向制动角度的比例系数。
 * @return PLATFORM_OK表示读取成功，参数为空时返回参数错误。
 */
int serial_monitor_ball_velocity_damping_get(float *velocity_damping);

/**
 * @brief  获取任务3纯定时开环控制参数快照。
 * @param  config 用于接收各段动作的角度、时间和最终保持角度。
 * @return PLATFORM_OK表示成功，参数为空时返回参数错误。
 */
int serial_monitor_task3_open_loop_config_get(
    struct serial_monitor_task3_open_loop_config *config);

/**
 * @brief  获取上位机配置的左右轮循迹基础速度快照。
 * @param  left_speed 用于接收左轮基础速度，单位为编码器计数每秒。
 * @param  right_speed 用于接收右轮基础速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   循迹任务读取配置并叠加差速，本函数不发布电机目标。
 */
int serial_monitor_drive_base_speeds_get(int32_t *left_speed,
                                         int32_t *right_speed);

/**
 * @brief  获取上位机请求的EMM绝对位置快照。
 * @param  position_deg 用于接收目标角度的变量。
 * @return PLATFORM_OK表示读取成功，参数为空时返回参数错误。
 * @note   单位为度；本函数不访问EMM或USART6硬件。
 */
int serial_monitor_emm_position_deg_get(float *position_deg);

#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
/**
 * @brief  发布左右轮实测速度，供 WEIXIN 波形协议低频发送。
 * @param  left_speed 左轮实测速度，单位为编码器计数每秒。
 * @param  right_speed 右轮实测速度，单位为编码器计数每秒。
 * @retval None。
 * @note   本函数只更新内存快照，不会在电机控制任务中执行串口发送。
 */
void serial_monitor_drive_speeds_publish(int32_t left_speed,
                                         int32_t right_speed);
#endif

#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
/**
 * @brief  发布小球原始速度与低通结果，供WEIXIN波形协议发送。
 * @param  velocity 小球原始速度，单位为像素每帧。
 * @param  filtered_velocity STM32估计并低通处理的速度，单位为像素每秒。
 * @retval None。
 * @note   本函数只更新内存快照，不访问USART1或USART6。
 */
void serial_monitor_ball_velocities_publish(int8_t velocity,
                                            float filtered_velocity);

/**
 * @brief  发布球控PID写入EMM邮箱的整数目标速度，供WEIXIN波形协议发送。
 * @param  speed_rpm PID浮点输出取整后的目标速度，单位为RPM。
 * @retval None。
 * @note   本函数只更新内存快照，不访问USART1或USART6。
 */
void serial_monitor_ball_speed_rpm_publish(int32_t speed_rpm);
#endif

#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
/**
 * @brief  发布小球位置环目标、测量值和误差，供WEIXIN波形协议发送。
 * @param  target_x 目标位置，单位为像素。
 * @param  measured_x 视觉测量位置，单位为像素。
 * @param  filtered_x 低通滤波后的位置，单位为像素。
 * @retval None。
 */
void serial_monitor_ball_positions_publish(int16_t target_x,
                                           int16_t measured_x,
                                           float filtered_x);
#endif

/**
 * @brief  运行 WEIXIN Serial Monitor UART1 通信任务。
 * @param  argument：FreeRTOS 任务参数，当前未使用。
 * @retval None。
 * @note   USART1 ISR只投递字节事件；本任务阻塞等待FreeRTOS队列并在任务上下文解析协议。
 */
void serial_monitor_task(void *argument);

#endif /* APP_SERIAL_MONITOR_H */
