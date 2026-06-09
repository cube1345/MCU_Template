#ifndef MYUSED_MYDEVICES_MOTOR_DRIVER_H
#define MYUSED_MYDEVICES_MOTOR_DRIVER_H

/**
 * @file motor_driver.h
 * @brief 直流电机 Device 层对象封装。
 */

#include <stdint.h>
#include "device_base.h"
#include "device_ops.h"
#include "hal_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOTOR_CHANNEL_A 0U
#define MOTOR_CHANNEL_B 1U
#define MOTOR_DEFAULT_PERIOD_TICKS 8000
#define MOTOR_DEFAULT_DEADZONE_PWM 250
#define MOTOR_DEFAULT_PWM_LIMIT 7000
#define MOTOR_DEFAULT_INTEGRAL_LIMIT 3000

typedef struct motor_s motor_t;
typedef device_motor_ops_t motor_base_ops_t;

typedef struct {
    platform_timer_t *timer;
    platform_timer_channel_t cc_forward;
    platform_timer_channel_t cc_reverse;
    uint8_t reverse_logic;
} motor_channel_hw_t;

typedef struct {
    motor_channel_hw_t left;
    motor_channel_hw_t right;
    int32_t period_ticks;
    int32_t deadzone_pwm;
    int32_t pwm_limit;
    int32_t integral_limit;
    float kp;
    float ki;
    float kd;
} motor_cfg_t;

struct motor_s {
    device_base_t base;
    const motor_base_ops_t *ops;
    motor_cfg_t cfg;
    int32_t integral_a;
    int32_t integral_b;
};

void Motor_InitDevice(motor_t *me, const motor_cfg_t *cfg);
void Motor_StartDevice(motor_t *me);
void Motor_StopDevice(motor_t *me);
void Motor_SetPwmDevice(motor_t *me, int32_t pwm_left, int32_t pwm_right);
int32_t Motor_VelocityCalcDevice(motor_t *me, uint8_t channel, int32_t target, int32_t current);

void Motor_Start(motor_t *me);
void Motor_Stop(motor_t *me);
void Motor_SetPwm(motor_t *me, int32_t pwml, int32_t pwmr);
int32_t Motor_VelocityCalc(motor_t *me, uint8_t channel, int32_t target, int32_t current);

int limit_PWM(int value, int low, int high);
int motor_apply_deadzone(int pwm);

#ifdef __cplusplus
}
#endif

#endif /* MYUSED_MYDEVICES_MOTOR_DRIVER_H */
