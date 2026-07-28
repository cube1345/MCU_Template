/**
 * @file motor_driver.c
 * @brief DC motor Device-layer implementation.
 * @details This file deliberately has no Board, SysConfig, or DriverLib
 *          dependency. Hardware writes are delegated to injected PWM ports.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#include "motor_driver.h"

static inline int32_t Motor_LimitValue(int32_t value, int32_t low, int32_t high)
{
    if (value > high) {
        return high;
    }
    if (value < low) {
        return low;
    }
    return value;
}

static inline int32_t Motor_ApplyDeadzoneValue(int32_t pwm, int32_t deadzone)
{
    if (pwm == 0) {
        return 0;
    }
    if ((pwm > 0) && (pwm < deadzone)) {
        return deadzone;
    }
    if ((pwm < 0) && (pwm > -deadzone)) {
        return -deadzone;
    }
    return pwm;
}

static uint8_t Motor_IsPortValid(const motor_channel_port_t *port)
{
    return (uint8_t)((port != 0) && (port->context != 0) &&
                     (port->start != 0) && (port->stop != 0) &&
                     (port->set_pwm != 0));
}

static void Motor_Base_Start(void *me)
{
    Motor_StartDevice((motor_t *)me);
}

static void Motor_Base_Stop(void *me)
{
    Motor_StopDevice((motor_t *)me);
}

static void Motor_Base_SetPwm(void *me, int32_t pwm_left, int32_t pwm_right)
{
    Motor_SetPwmDevice((motor_t *)me, pwm_left, pwm_right);
}

static int32_t Motor_Base_VelocityCalc(void *me, uint8_t channel, int32_t target, int32_t current)
{
    return Motor_VelocityCalcDevice((motor_t *)me, channel, target, current);
}

static const motor_base_ops_t g_motor_base_ops = {
    .start = Motor_Base_Start,
    .stop = Motor_Base_Stop,
    .set_pwm = Motor_Base_SetPwm,
    .velocity_calc = Motor_Base_VelocityCalc,
};

void Motor_InitDevice(motor_t *me, const motor_cfg_t *cfg)
{
    if (me == 0) {
        return;
    }

    me->base.is_init = 0U;
    me->ops = 0;
    if ((cfg == 0) || (Motor_IsPortValid(&cfg->left) == 0U) ||
        (Motor_IsPortValid(&cfg->right) == 0U) || (cfg->pwm_limit <= 0) ||
        (cfg->deadzone_pwm < 0) || (cfg->deadzone_pwm > cfg->pwm_limit) ||
        (cfg->integral_limit < 0)) {
        return;
    }

    me->cfg = *cfg;
    me->integral_a = 0;
    me->integral_b = 0;
    me->ops = &g_motor_base_ops;
    me->base.is_init = 1U;
}

void Motor_StartDevice(motor_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    me->cfg.left.start(me->cfg.left.context);
    me->cfg.right.start(me->cfg.right.context);
}

void Motor_StopDevice(motor_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    Motor_SetPwmDevice(me, 0, 0);
    me->cfg.left.stop(me->cfg.left.context);
    me->cfg.right.stop(me->cfg.right.context);
}

void Motor_SetPwmDevice(motor_t *me, int32_t pwm_left, int32_t pwm_right)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    pwm_left = Motor_LimitValue(pwm_left, -me->cfg.pwm_limit, me->cfg.pwm_limit);
    pwm_right = Motor_LimitValue(pwm_right, -me->cfg.pwm_limit, me->cfg.pwm_limit);
    pwm_left = Motor_ApplyDeadzoneValue(pwm_left, me->cfg.deadzone_pwm);
    pwm_right = Motor_ApplyDeadzoneValue(pwm_right, me->cfg.deadzone_pwm);

    me->cfg.right.set_pwm(me->cfg.right.context, pwm_right);
    me->cfg.left.set_pwm(me->cfg.left.context, pwm_left);
}

int32_t Motor_VelocityCalcDevice(motor_t *me, uint8_t channel, int32_t target, int32_t current)
{
    int32_t bias;
    int32_t *integral;
    int32_t output;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return 0;
    }

    integral = (channel == MOTOR_CHANNEL_B) ? &me->integral_b : &me->integral_a;
    bias = target - current;
    *integral += bias;
    *integral = Motor_LimitValue(*integral, -me->cfg.integral_limit, me->cfg.integral_limit);

    output = (int32_t)((me->cfg.kp * (float)bias) + (me->cfg.ki * (float)(*integral)));
    return Motor_LimitValue(output, -me->cfg.pwm_limit, me->cfg.pwm_limit);
}

void Motor_Start(motor_t *me)
{
    if ((me != 0) && (me->ops != 0) && (me->ops->start != 0)) {
        me->ops->start(me);
    }
}

void Motor_Stop(motor_t *me)
{
    if ((me != 0) && (me->ops != 0) && (me->ops->stop != 0)) {
        me->ops->stop(me);
    }
}

void Motor_SetPwm(motor_t *me, int32_t pwm_left, int32_t pwm_right)
{
    if ((me != 0) && (me->ops != 0) && (me->ops->set_pwm != 0)) {
        me->ops->set_pwm(me, pwm_left, pwm_right);
    }
}

int32_t Motor_VelocityCalc(motor_t *me, uint8_t channel, int32_t target, int32_t current)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->velocity_calc == 0)) {
        return 0;
    }
    return me->ops->velocity_calc(me, channel, target, current);
}
