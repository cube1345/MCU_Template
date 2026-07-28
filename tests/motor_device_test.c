/**
 * @file motor_device_test.c
 * @brief Host test for the board-injected Motor Device PWM ports.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#include <assert.h>
#include <stdint.h>

#include "motor_driver.h"

typedef struct {
    uint32_t start_count;
    uint32_t stop_count;
    int32_t last_pwm;
} motor_test_port_t;

static void MotorTest_Start(const void *context)
{
    motor_test_port_t *port = (motor_test_port_t *)context;

    ++port->start_count;
}

static void MotorTest_Stop(const void *context)
{
    motor_test_port_t *port = (motor_test_port_t *)context;

    ++port->stop_count;
}

static void MotorTest_SetPwm(const void *context, int32_t pwm)
{
    motor_test_port_t *port = (motor_test_port_t *)context;

    port->last_pwm = pwm;
}

static motor_channel_port_t MotorTest_CreatePort(motor_test_port_t *port)
{
    motor_channel_port_t channel = {
        .context = port,
        .start = MotorTest_Start,
        .stop = MotorTest_Stop,
        .set_pwm = MotorTest_SetPwm,
    };

    return channel;
}

int main(void)
{
    motor_test_port_t left = {0};
    motor_test_port_t right = {0};
    motor_t motor = {0};
    motor_cfg_t cfg = {
        .left = MotorTest_CreatePort(&left),
        .right = MotorTest_CreatePort(&right),
        .deadzone_pwm = 100,
        .pwm_limit = 500,
        .integral_limit = 200,
        .kp = 2.0f,
        .ki = 1.0f,
    };

    Motor_InitDevice(&motor, &cfg);
    assert(motor.base.is_init == 1U);

    Motor_Start(&motor);
    assert(left.start_count == 1U);
    assert(right.start_count == 1U);

    Motor_SetPwm(&motor, 50, -900);
    assert(left.last_pwm == 100);
    assert(right.last_pwm == -500);

    assert(Motor_VelocityCalc(&motor, MOTOR_CHANNEL_A, 10, 0) == 30);
    assert(Motor_VelocityCalc(&motor, MOTOR_CHANNEL_A, 10, 0) == 40);

    Motor_Stop(&motor);
    assert(left.last_pwm == 0);
    assert(right.last_pwm == 0);
    assert(left.stop_count == 1U);
    assert(right.stop_count == 1U);

    cfg.left.set_pwm = 0;
    Motor_InitDevice(&motor, &cfg);
    assert(motor.base.is_init == 0U);

    return 0;
}
