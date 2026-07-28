/**
 * @file motor_facade.c
 * @brief Default-instance and compatibility facade for the DC motor.
 * @details Keeps legacy global APIs outside the reusable Device layer.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#include "motor_facade.h"

#include "../3_board/motor_board.h"

void Motor_Init(void)
{
    Motor_Board_Init();
}

int Velocity_A(int target_velocity, int current_velocity)
{
    motor_t *motor = Motor_Board_GetDefault();

    return (int)Motor_VelocityCalc(motor, MOTOR_CHANNEL_A, target_velocity, current_velocity);
}

int Velocity_B(int target_velocity, int current_velocity)
{
    motor_t *motor = Motor_Board_GetDefault();

    return (int)Motor_VelocityCalc(motor, MOTOR_CHANNEL_B, target_velocity, current_velocity);
}

void Set_PWM(int pwm_left, int pwm_right)
{
    Motor_SetPwm(Motor_Board_GetDefault(), pwm_left, pwm_right);
}
