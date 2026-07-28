/**
 * @file motor_driver.h
 * @brief Board-independent DC motor control device.
 * @details Owns PWM limiting, deadzone compensation, and PI state. PWM timer
 *          access is supplied by Board through motor_channel_port_t.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>

#include "device_base.h"
#include "device_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOTOR_CHANNEL_A               (0U)
#define MOTOR_CHANNEL_B               (1U)
#define MOTOR_DEFAULT_DEADZONE_PWM    (250)
#define MOTOR_DEFAULT_PWM_LIMIT       (7000)
#define MOTOR_DEFAULT_INTEGRAL_LIMIT  (3000)

typedef struct motor_s motor_t;
typedef device_motor_ops_t motor_base_ops_t;

/**
 * @brief Board-injected PWM channel operations.
 * @details set_pwm receives a signed, already limited PWM value. Callbacks run
 *          in the caller context and must not allocate memory.
 */
typedef struct {
    const void *context;
    void (*start)(const void *context);
    void (*stop)(const void *context);
    void (*set_pwm)(const void *context, int32_t pwm);
} motor_channel_port_t;

/**
 * @brief Configuration for one DC motor object.
 */
typedef struct {
    motor_channel_port_t left;
    motor_channel_port_t right;
    int32_t deadzone_pwm;
    int32_t pwm_limit;
    int32_t integral_limit;
    float kp;
    float ki;
} motor_cfg_t;

struct motor_s {
    device_base_t base;
    const motor_base_ops_t *ops;
    motor_cfg_t cfg;
    int32_t integral_a;
    int32_t integral_b;
};

/**
 * @brief Motor_InitDevice - Bind a motor object to Board PWM ports.
 * @param[in,out] me Motor instance to initialize.
 * @param[in] cfg Immutable motor configuration and port callbacks.
 * @return None. An invalid configuration leaves the object uninitialized.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */
void Motor_InitDevice(motor_t *me, const motor_cfg_t *cfg);

/** @brief Start the two board-provided PWM channels for a ready motor. */
void Motor_StartDevice(motor_t *me);

/** @brief Stop PWM output after commanding zero duty. */
void Motor_StopDevice(motor_t *me);

/** @brief Set signed left and right PWM commands after limit/deadzone handling. */
void Motor_SetPwmDevice(motor_t *me, int32_t pwm_left, int32_t pwm_right);

/** @brief Calculate one PI velocity-control output and retain its integral state. */
int32_t Motor_VelocityCalcDevice(motor_t *me, uint8_t channel, int32_t target, int32_t current);

/** @brief Dispatch the generic start operation through the motor ops table. */
void Motor_Start(motor_t *me);

/** @brief Dispatch the generic stop operation through the motor ops table. */
void Motor_Stop(motor_t *me);

/** @brief Dispatch the generic PWM operation through the motor ops table. */
void Motor_SetPwm(motor_t *me, int32_t pwm_left, int32_t pwm_right);

/** @brief Dispatch the generic PI velocity operation through the motor ops table. */
int32_t Motor_VelocityCalc(motor_t *me, uint8_t channel, int32_t target, int32_t current);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_DRIVER_H */
