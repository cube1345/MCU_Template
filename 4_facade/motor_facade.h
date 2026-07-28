/**
 * @file motor_facade.h
 * @brief Stable application and legacy entry points for the default motor.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#ifndef MOTOR_FACADE_H
#define MOTOR_FACADE_H

#include "../2_device/motor_driver.h"

/** @brief Initialize the default board motor. */
void Motor_Init(void);

/** @brief Return the PI output for the legacy left motor channel. */
int Velocity_A(int target_velocity, int current_velocity);

/** @brief Return the PI output for the legacy right motor channel. */
int Velocity_B(int target_velocity, int current_velocity);

/** @brief Apply signed PWM to the default left and right channels. */
void Set_PWM(int pwm_left, int pwm_right);

#endif /* MOTOR_FACADE_H */
