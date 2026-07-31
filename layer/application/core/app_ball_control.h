/* SPDX-License-Identifier: MIT */
#ifndef APP_BALL_CONTROL_H
#define APP_BALL_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/* Ball target range relative to the tube centre, in pixels. */
#define BALL_CONTROL_TARGET_X_MIN INT16_C(-320)
#define BALL_CONTROL_TARGET_X_MAX INT16_C(320)

/* Outer position-loop defaults. Its output unit is pixels per second. */
#define BALL_CONTROL_POSITION_KP_DEFAULT (0.5F)
#define BALL_CONTROL_POSITION_KI_DEFAULT (0.0F)
#define BALL_CONTROL_POSITION_KD_DEFAULT (0.0F)

/* Inner velocity-loop defaults. Its output unit is EMM angle degrees. */
#define BALL_CONTROL_VELOCITY_KP_DEFAULT (0.06F)
#define BALL_CONTROL_VELOCITY_KI_DEFAULT (0.0F)
#define BALL_CONTROL_VELOCITY_KD_DEFAULT (0.0F)

struct ball_control_status {
  int16_t target_x;
  int16_t measured_x;
  float filtered_x;
  int8_t measured_velocity;
  float filtered_velocity;
  int8_t measured_acceleration;
  uint32_t vision_sequence;
  /* Position PID output and velocity PID target, in pixels per second. */
  float target_velocity;
  /* target_velocity minus filtered_velocity, in pixels per second. */
  float velocity_error;
  /* Velocity PID output and EMM absolute target, in degrees. */
  float output_position_deg;
  bool arrived;
  bool enabled;
  bool vision_valid;
};

int ball_control_target_x_set(int16_t target_x);
int ball_control_enabled_set(bool enabled);
int ball_control_status_get(struct ball_control_status *status);
void ball_control_task(void *argument);

#endif /* APP_BALL_CONTROL_H */
