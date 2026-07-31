/* SPDX-License-Identifier: MIT */
/**
 * @file app_ball_control.c
 * @brief Cascaded ball position and velocity control task.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BALL_CONTROL_PERIOD_MS UINT32_C(20)
#define BALL_CONTROL_DT_S (0.020F)
#define BALL_CONTROL_PRINT_PERIOD_MS UINT32_C(100)
#define BALL_CONTROL_VISION_TIMEOUT_MS UINT32_C(200)

#define BALL_CONTROL_ARRIVAL_POSITION_TOLERANCE_PX (20.0F)
#define BALL_CONTROL_ARRIVAL_VELOCITY_TOLERANCE_PX_S (20.0F)
#define BALL_CONTROL_ARRIVAL_CONFIRM_FRAMES UINT8_C(5)

#define BALL_CONTROL_POSITION_FILTER_ALPHA (0.8F)
#define BALL_CONTROL_VELOCITY_FILTER_ALPHA (0.3F)
#define BALL_CONTROL_TARGET_VELOCITY_LIMIT_PX_S (250.0F)
#define BALL_CONTROL_OUTPUT_ANGLE_LIMIT_DEG (40.0F)
/* 内环调试开关：1时旁路位置外环，并固定目标球速。 */
#define BALL_CONTROL_FIXED_TARGET_VELOCITY_ENABLE 0
#define BALL_CONTROL_FIXED_TARGET_VELOCITY_PX_S (140.0F)

static const struct pid_config s_position_pid_config = {
  .kp = BALL_CONTROL_POSITION_KP_DEFAULT,
  .ki = BALL_CONTROL_POSITION_KI_DEFAULT,
  .kd = BALL_CONTROL_POSITION_KD_DEFAULT,
  .output_min = -BALL_CONTROL_TARGET_VELOCITY_LIMIT_PX_S,
  .output_max = BALL_CONTROL_TARGET_VELOCITY_LIMIT_PX_S,
  .output_deadband = 0.0F,
  .integral_clear_error = 5.0F,
  .integral_min = -100.0F,
  .integral_max = 100.0F,
};

static const struct pid_config s_velocity_pid_config = {
  .kp = BALL_CONTROL_VELOCITY_KP_DEFAULT,
  .ki = BALL_CONTROL_VELOCITY_KI_DEFAULT,
  .kd = BALL_CONTROL_VELOCITY_KD_DEFAULT,
  .output_min = -BALL_CONTROL_OUTPUT_ANGLE_LIMIT_DEG,
  .output_max = BALL_CONTROL_OUTPUT_ANGLE_LIMIT_DEG,
  .output_deadband = 0.0F,
  .integral_clear_error = 5.0F,
  .integral_min = -30.0F,
  .integral_max = 30.0F,
};

static bool s_control_enabled;
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

static void ball_control_settings_get(int16_t *target_x, bool *enabled)
{
  (void)serial_monitor_ball_target_x_get(target_x);
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

static bool ball_control_gains_changed(float requested_kp,
                                       float requested_ki,
                                       float requested_kd,
                                       float applied_kp,
                                       float applied_ki,
                                       float applied_kd)
{
  return (requested_kp != applied_kp) ||
         (requested_ki != applied_ki) ||
         (requested_kd != applied_kd);
}

void ball_control_task(void *argument)
{
  struct ball_control_status status = {0};
  struct vision_ball_data ball;
  struct pid position_controller;
  struct pid velocity_controller;
  struct pid_config position_config = s_position_pid_config;
  struct pid_config velocity_config = s_velocity_pid_config;
  struct low_pass_filter position_filter;
  struct low_pass_filter velocity_filter;
  TickType_t last_wake_time;
  TickType_t last_update_tick;
  TickType_t last_print_tick;
  TickType_t last_vision_tick;
  TickType_t current_tick;
  float update_dt_s;
  float previous_filtered_x = 0.0F;
  float raw_velocity_px_s;
  float position_error;
  float requested_position_kp;
  float requested_position_ki;
  float requested_position_kd;
  float requested_velocity_kp;
  float requested_velocity_ki;
  float requested_velocity_kd;
  float applied_position_kp = BALL_CONTROL_POSITION_KP_DEFAULT;
  float applied_position_ki = BALL_CONTROL_POSITION_KI_DEFAULT;
  float applied_position_kd = BALL_CONTROL_POSITION_KD_DEFAULT;
  float applied_velocity_kp = BALL_CONTROL_VELOCITY_KP_DEFAULT;
  float applied_velocity_ki = BALL_CONTROL_VELOCITY_KI_DEFAULT;
  float applied_velocity_kd = BALL_CONTROL_VELOCITY_KD_DEFAULT;
  uint32_t last_seen_sequence = UINT32_C(0);
  uint32_t last_processed_sequence = UINT32_C(0);
  int16_t applied_target_x = INT16_MIN;
  uint8_t arrival_confirm_count = UINT8_C(0);
  bool velocity_estimate_initialized = false;
  bool vision_frame_seen = false;
  bool processed_frame_seen = false;
  bool new_frame;
  int result;

  (void)argument;
  result = pid_init(&position_controller, &position_config);
  if (result == PLATFORM_OK) {
    result = pid_init(&velocity_controller, &velocity_config);
  }
  if (result == PLATFORM_OK) {
    result = low_pass_filter_init(&position_filter,
                                  BALL_CONTROL_POSITION_FILTER_ALPHA);
  }
  if (result == PLATFORM_OK) {
    result = low_pass_filter_init(&velocity_filter,
                                  BALL_CONTROL_VELOCITY_FILTER_ALPHA);
  }
  if (result != PLATFORM_OK) {
    printf("[ball_ctrl] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  last_update_tick = last_wake_time;
  last_print_tick = last_wake_time;
  last_vision_tick = last_wake_time;

  for (;;) {
    current_tick = xTaskGetTickCount();
    ball_control_settings_get(&status.target_x, &status.enabled);

    result = serial_monitor_ball_position_gains_get(
        &requested_position_kp, &requested_position_ki,
        &requested_position_kd);
    if ((result == PLATFORM_OK) && ball_control_gains_changed(
            requested_position_kp, requested_position_ki,
            requested_position_kd, applied_position_kp,
            applied_position_ki, applied_position_kd)) {
      position_config.kp = requested_position_kp;
      position_config.ki = requested_position_ki;
      position_config.kd = requested_position_kd;
      result = pid_init(&position_controller, &position_config);
      if (result == PLATFORM_OK) {
        applied_position_kp = requested_position_kp;
        applied_position_ki = requested_position_ki;
        applied_position_kd = requested_position_kd;
      }
    }
    if (result == PLATFORM_OK) {
      result = serial_monitor_ball_velocity_gains_get(
          &requested_velocity_kp, &requested_velocity_ki,
          &requested_velocity_kd);
    }
    if ((result == PLATFORM_OK) && ball_control_gains_changed(
            requested_velocity_kp, requested_velocity_ki,
            requested_velocity_kd, applied_velocity_kp,
            applied_velocity_ki, applied_velocity_kd)) {
      velocity_config.kp = requested_velocity_kp;
      velocity_config.ki = requested_velocity_ki;
      velocity_config.kd = requested_velocity_kd;
      result = pid_init(&velocity_controller, &velocity_config);
      if (result == PLATFORM_OK) {
        applied_velocity_kp = requested_velocity_kp;
        applied_velocity_ki = requested_velocity_ki;
        applied_velocity_kd = requested_velocity_kd;
      }
    }

    if (status.target_x != applied_target_x) {
      applied_target_x = status.target_x;
      pid_reset(&position_controller);
      pid_reset(&velocity_controller);
      arrival_confirm_count = UINT8_C(0);
      status.arrived = false;
    }

    new_frame = false;
    if (result == PLATFORM_OK) {
      result = vision_get_ball_data(&ball);
    }
    if ((result == PLATFORM_OK) && ball.valid) {
      status.measured_x = ball.x;
      status.measured_velocity = ball.velocity;
      status.measured_acceleration = ball.acceleration;
      status.vision_sequence = ball.sequence;
      if (!vision_frame_seen || (ball.sequence != last_seen_sequence)) {
        vision_frame_seen = true;
        last_seen_sequence = ball.sequence;
        last_vision_tick = current_tick;
        new_frame = true;
      }
    }
    status.vision_valid =
        (result == PLATFORM_OK) && ball.valid && vision_frame_seen &&
        ((current_tick - last_vision_tick) <=
         pdMS_TO_TICKS(BALL_CONTROL_VISION_TIMEOUT_MS));

    if (!status.enabled || !status.vision_valid) {
      pid_reset(&position_controller);
      pid_reset(&velocity_controller);
      (void)low_pass_filter_init(
          &position_filter, BALL_CONTROL_POSITION_FILTER_ALPHA);
      (void)low_pass_filter_init(
          &velocity_filter, BALL_CONTROL_VELOCITY_FILTER_ALPHA);
      status.filtered_x = 0.0F;
      status.filtered_velocity = 0.0F;
      status.target_velocity = 0.0F;
      status.velocity_error = 0.0F;
      status.output_position_deg = 0.0F;
      status.arrived = false;
      arrival_confirm_count = UINT8_C(0);
      velocity_estimate_initialized = false;
      last_update_tick = current_tick;
    } else if (new_frame &&
               (!processed_frame_seen ||
                (status.vision_sequence != last_processed_sequence))) {
      update_dt_s = (float)(current_tick - last_update_tick) /
                    (float)configTICK_RATE_HZ;
      if (!velocity_estimate_initialized || (update_dt_s <= 0.0F)) {
        update_dt_s = BALL_CONTROL_DT_S;
      }

      result = low_pass_filter_update(
          &position_filter, (float)status.measured_x,
          &status.filtered_x);
      if (result == PLATFORM_OK) {
        if (!velocity_estimate_initialized) {
          previous_filtered_x = status.filtered_x;
          velocity_estimate_initialized = true;
          result = low_pass_filter_reset(&velocity_filter, 0.0F);
          status.filtered_velocity = 0.0F;
        } else {
          raw_velocity_px_s =
              (status.filtered_x - previous_filtered_x) / update_dt_s;
          previous_filtered_x = status.filtered_x;
          result = low_pass_filter_update(
              &velocity_filter, raw_velocity_px_s,
              &status.filtered_velocity);
        }
      }

      position_error = (float)status.target_x - status.filtered_x;
      if (result == PLATFORM_OK) {
#if (BALL_CONTROL_FIXED_TARGET_VELOCITY_ENABLE == 1)
        status.target_velocity = BALL_CONTROL_FIXED_TARGET_VELOCITY_PX_S;
#else
        result = pid_update(&position_controller, position_error,
                            update_dt_s, &status.target_velocity);
#endif
      }
      if (result == PLATFORM_OK) {
        status.velocity_error = status.target_velocity -
                                status.filtered_velocity;
        result = pid_update(&velocity_controller,
                            status.velocity_error, update_dt_s,
                            &status.output_position_deg);
      }

      if ((result == PLATFORM_OK) &&
          (math_abs_float(position_error) <=
           BALL_CONTROL_ARRIVAL_POSITION_TOLERANCE_PX) &&
          (math_abs_float(status.filtered_velocity) <=
           BALL_CONTROL_ARRIVAL_VELOCITY_TOLERANCE_PX_S)) {
        if (arrival_confirm_count <
            BALL_CONTROL_ARRIVAL_CONFIRM_FRAMES) {
          arrival_confirm_count++;
        }
      } else {
        arrival_confirm_count = UINT8_C(0);
      }
      status.arrived =
          (arrival_confirm_count >= BALL_CONTROL_ARRIVAL_CONFIRM_FRAMES);

      if (result != PLATFORM_OK) {
        pid_reset(&position_controller);
        pid_reset(&velocity_controller);
        status.target_velocity = 0.0F;
        status.velocity_error = 0.0F;
        status.output_position_deg = 0.0F;
        status.arrived = false;
        arrival_confirm_count = UINT8_C(0);
        velocity_estimate_initialized = false;
      }
      last_processed_sequence = status.vision_sequence;
      processed_frame_seen = true;
      last_update_tick = current_tick;
    }

    result = emm_v5_target_position_deg_set(status.output_position_deg);
    if (result != PLATFORM_OK) {
      pid_reset(&position_controller);
      pid_reset(&velocity_controller);
      status.target_velocity = 0.0F;
      status.velocity_error = 0.0F;
      status.output_position_deg = 0.0F;
      status.arrived = false;
      arrival_confirm_count = UINT8_C(0);
      (void)emm_v5_target_position_deg_set(0.0F);
    }
    ball_control_status_set(&status);

#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
    if (status.vision_valid) {
      serial_monitor_ball_velocities_publish(
          status.target_velocity, status.filtered_velocity);
    }
#endif
#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
    if (status.vision_valid) {
      serial_monitor_ball_positions_publish(
          status.target_x, status.measured_x, status.filtered_x);
    }
#endif

    if ((current_tick - last_print_tick) >=
        pdMS_TO_TICKS(BALL_CONTROL_PRINT_PERIOD_MS)) {
      printf("[ball_ctrl] target_x=%d filtered_x=%.3f error_x=%.3f "
             "target_v=%.3f filtered_v=%.3f error_v=%.3f "
             "output_deg=%.3f arrived=%u valid=%u\r\n",
             (int)status.target_x,
             (double)status.filtered_x,
             (double)((float)status.target_x - status.filtered_x),
             (double)status.target_velocity,
             (double)status.filtered_velocity,
             (double)status.velocity_error,
             (double)status.output_position_deg,
             status.arrived ? 1U : 0U,
             status.vision_valid ? 1U : 0U);
      last_print_tick = current_tick;
    }

    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(BALL_CONTROL_PERIOD_MS));
  }
}
