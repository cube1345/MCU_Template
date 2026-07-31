/* SPDX-License-Identifier: MIT */
/**
 * @file    app_ball_control.c
 * @brief   视觉小球位置闭环控制任务。
 * @author  weixin
 */
#include "app_ball_control.h"

#include "app_emm_v5.h"
#include "app_serial_monitor.h"
#include "app_vision.h"
#include "low_pass_filter.h"
#include "math_utils.h"
#include "pid.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BALL_CONTROL_PERIOD_MS UINT32_C(20)
#define BALL_CONTROL_DT_S (0.020F)
#define BALL_CONTROL_PRINT_PERIOD_MS UINT32_C(100)
#define BALL_CONTROL_ARRIVAL_POSITION_TOLERANCE_PX (20.0F)
#define BALL_CONTROL_ARRIVAL_VELOCITY_TOLERANCE_PX_S (20.0F)
/* 小球位置低通滤波中新样本的权重，越小越平滑但位置环响应越慢。 */
#define BALL_CONTROL_POSITION_FILTER_ALPHA (0.25F)
/* STM32位置差分速度低通滤波的新样本权重。 */
#define BALL_CONTROL_VELOCITY_FILTER_ALPHA (0.25F)

/* 位置误差到EMM绝对角度的PID参数，机械方向和增益需通过整机调试确认。 */
static const struct pid_config s_position_pid_config = {
  .kp = BALL_CONTROL_POSITION_KP_DEFAULT,
  .ki = BALL_CONTROL_POSITION_KI_DEFAULT,
  .kd = BALL_CONTROL_POSITION_KD_DEFAULT,
  .output_min = -40.0F,
  .output_max = 40.0F,
  .output_deadband = 2.0F,
  .integral_clear_error = 0.0F,
  .integral_min = -20.0F,
  .integral_max = 20.0F,
};

/* 控制默认启用，PID输出通过绝对角度邮箱交给EMM任务执行。 */
static bool s_control_enabled = true;
static struct ball_control_status s_control_status;

int ball_control_target_x_set(int16_t target_x)
{
  return serial_monitor_ball_target_x_set(target_x);
}

int ball_control_enabled_set(bool enabled)
{
  taskENTER_CRITICAL();
  s_control_enabled = enabled;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

int ball_control_status_get(struct ball_control_status *status)
{
  if (status == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  taskENTER_CRITICAL();
  *status = s_control_status;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

static void ball_control_settings_get(int16_t *target_x,
                                      float *velocity_damping,
                                      bool *enabled)
{
  (void)serial_monitor_ball_target_x_get(target_x);
  (void)serial_monitor_ball_velocity_damping_get(velocity_damping);
  taskENTER_CRITICAL();
  *enabled = s_control_enabled;
  taskEXIT_CRITICAL();
}

static void ball_control_status_set(
    const struct ball_control_status *status)
{
  taskENTER_CRITICAL();
  s_control_status = *status;
  taskEXIT_CRITICAL();
}

void ball_control_task(void *argument)
{
  struct ball_control_status status = {0};
  struct vision_ball_data ball;
  struct pid position_controller;
  struct pid_config position_pid_config = s_position_pid_config;
  struct low_pass_filter position_filter;
  struct low_pass_filter velocity_filter;
  TickType_t last_wake_time;
  TickType_t last_update_tick;
  TickType_t last_print_tick;
  TickType_t current_tick;
  float update_dt_s;
  float previous_filtered_x = 0.0F;
  float raw_velocity_px_s;
  float position_error;
  float velocity_damping;
  float requested_position_kp;
  float requested_position_ki;
  float requested_position_kd;
  float applied_position_kp = BALL_CONTROL_POSITION_KP_DEFAULT;
  float applied_position_ki = BALL_CONTROL_POSITION_KI_DEFAULT;
  float applied_position_kd = BALL_CONTROL_POSITION_KD_DEFAULT;
  uint32_t last_sequence = UINT32_C(0);
  bool velocity_estimate_initialized = false;
  int result;

  (void)argument;
  result = pid_init(&position_controller, &s_position_pid_config);
  if (result != PLATFORM_OK) {
    vTaskDelete(NULL);
    return;
  }
  result = low_pass_filter_init(&velocity_filter,
                                BALL_CONTROL_VELOCITY_FILTER_ALPHA);
  if (result != PLATFORM_OK) {
    vTaskDelete(NULL);
    return;
  }
  result = low_pass_filter_init(&position_filter,
                                BALL_CONTROL_POSITION_FILTER_ALPHA);
  if (result != PLATFORM_OK) {
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  last_update_tick = last_wake_time;
  last_print_tick = last_wake_time;
  for (;;) {
    ball_control_settings_get(&status.target_x, &velocity_damping,
                              &status.enabled);
    result = serial_monitor_ball_position_gains_get(
        &requested_position_kp, &requested_position_ki,
        &requested_position_kd);
    if (result == PLATFORM_OK &&
        ((requested_position_kp != applied_position_kp) ||
         (requested_position_ki != applied_position_ki) ||
         (requested_position_kd != applied_position_kd))) {
      position_pid_config.kp = requested_position_kp;
      position_pid_config.ki = requested_position_ki;
      position_pid_config.kd = requested_position_kd;
      result = pid_init(&position_controller, &position_pid_config);
      if (result == PLATFORM_OK) {
        applied_position_kp = requested_position_kp;
        applied_position_ki = requested_position_ki;
        applied_position_kd = requested_position_kd;
        status.position_pid_output_deg = 0.0F;
        status.velocity_damping_deg = 0.0F;
        status.output_position_deg = 0.0F;
      }
    }
    result = vision_get_ball_data(&ball);
    status.vision_valid = (result == PLATFORM_OK) && ball.valid;
    if (status.vision_valid) {
      status.measured_x = ball.x;
      status.measured_velocity = ball.velocity;
      status.measured_acceleration = ball.acceleration;
      status.vision_sequence = ball.sequence;
    }

    if (!status.enabled || !status.vision_valid) {
      pid_reset(&position_controller);
      (void)low_pass_filter_init(
          &position_filter, BALL_CONTROL_POSITION_FILTER_ALPHA);
      (void)low_pass_filter_init(
          &velocity_filter, BALL_CONTROL_VELOCITY_FILTER_ALPHA);
      status.filtered_x = 0.0F;
      status.filtered_velocity = 0.0F;
      status.position_pid_output_deg = 0.0F;
      status.velocity_damping_deg = 0.0F;
      status.output_position_deg = 0.0F;
      status.arrived = false;
      velocity_estimate_initialized = false;
      last_sequence = status.vision_sequence;
      last_update_tick = xTaskGetTickCount();
    } else if (ball.sequence != last_sequence) {
      current_tick = xTaskGetTickCount();
      update_dt_s = (float)(current_tick - last_update_tick) /
                    (float)configTICK_RATE_HZ;
      if (last_sequence == UINT32_C(0)) {
        update_dt_s = BALL_CONTROL_DT_S;
      }
      result = low_pass_filter_update(
          &position_filter, (float)ball.x,
          &status.filtered_x);
      if (result == PLATFORM_OK) {
        if (!velocity_estimate_initialized) {
          previous_filtered_x = status.filtered_x;
          velocity_estimate_initialized = true;
          result = low_pass_filter_reset(
              &velocity_filter, 0.0F);
          status.filtered_velocity = 0.0F;
        } else {
          raw_velocity_px_s =
              (status.filtered_x - previous_filtered_x) /
              update_dt_s;
          previous_filtered_x = status.filtered_x;
          result = low_pass_filter_update(
              &velocity_filter, raw_velocity_px_s,
              &status.filtered_velocity);
        }
      }
      if (result == PLATFORM_OK) {
        position_error = (float)status.target_x -
                         status.filtered_x;
        status.arrived =
            (math_abs_float(position_error) <=
             BALL_CONTROL_ARRIVAL_POSITION_TOLERANCE_PX) &&
            (math_abs_float(status.filtered_velocity) <=
             BALL_CONTROL_ARRIVAL_VELOCITY_TOLERANCE_PX_S);
        if (status.arrived) {
          pid_reset(&position_controller);
          status.position_pid_output_deg = 0.0F;
          status.velocity_damping_deg = 0.0F;
          status.output_position_deg = 0.0F;
        } else {
          result = pid_update(&position_controller,
                              position_error,
                              update_dt_s,
                              &status.position_pid_output_deg);
        }
      }
      if ((result == PLATFORM_OK) && !status.arrived) {
        status.velocity_damping_deg =
            velocity_damping * status.filtered_velocity;
        status.output_position_deg = math_clamp_float(
            status.position_pid_output_deg -
                status.velocity_damping_deg,
            s_position_pid_config.output_min,
            s_position_pid_config.output_max);
      }
      if (result != PLATFORM_OK) {
        pid_reset(&position_controller);
        status.position_pid_output_deg = 0.0F;
        status.velocity_damping_deg = 0.0F;
        status.output_position_deg = 0.0F;
        status.arrived = false;
        velocity_estimate_initialized = false;
      }
      last_sequence = ball.sequence;
      last_update_tick = current_tick;
    }

    /* PID浮点输出取整后写入邮箱，USART6命令由EMM任务串行发送。 */
    result = emm_v5_target_position_deg_set(status.output_position_deg);
    if (result != PLATFORM_OK) {
      pid_reset(&position_controller);
      status.position_pid_output_deg = 0.0F;
      status.velocity_damping_deg = 0.0F;
      status.output_position_deg = 0.0F;
      status.arrived = false;
      (void)emm_v5_target_position_deg_set(0.0F);
    }
    ball_control_status_set(&status);
#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
    if (status.vision_valid) {
      serial_monitor_ball_velocities_publish(
          status.measured_velocity, status.filtered_velocity);
    }
#endif
#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
    if (status.vision_valid) {
      serial_monitor_ball_positions_publish(
          status.target_x, status.measured_x, status.filtered_x);
    }
#endif
    current_tick = xTaskGetTickCount();
    if ((current_tick - last_print_tick) >=
        pdMS_TO_TICKS(BALL_CONTROL_PRINT_PERIOD_MS)) {
      printf("[ball_ctrl] target_x=%d measured_x=%d filtered_x=%.3f "
             "error_x=%.3f "
             "reported_v=%d estimated_v_px_s=%.3f position_pid_deg=%.3f "
             "damping_deg=%.3f output_position_deg=%.3f "
             "arrived=%u valid=%u\r\n",
             (int)status.target_x,
             (int)status.measured_x,
             (double)status.filtered_x,
             (double)((float)status.target_x - status.filtered_x),
             (int)status.measured_velocity,
             (double)status.filtered_velocity,
             (double)status.position_pid_output_deg,
             (double)status.velocity_damping_deg,
             (double)status.output_position_deg,
             status.arrived ? 1U : 0U,
             status.vision_valid ? 1U : 0U);
      last_print_tick = current_tick;
    }
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(BALL_CONTROL_PERIOD_MS));
  }
}
