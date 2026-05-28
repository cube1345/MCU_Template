#include "motor_board.h"
#include "ti_msp_dl_config.h"

static motor_t g_default_motor;

static const motor_cfg_t g_default_motor_cfg = {
    .left = {
        .timer = PWM_1_INST,
        .cc_forward = GPIO_PWM_1_C0_IDX,
        .cc_reverse = GPIO_PWM_1_C1_IDX,
        .reverse_logic = 1U,
    },
    .right = {
        .timer = PWM_0_INST,
        .cc_forward = GPIO_PWM_0_C0_IDX,
        .cc_reverse = GPIO_PWM_0_C1_IDX,
        .reverse_logic = 0U,
    },
    .period_ticks = MOTOR_DEFAULT_PERIOD_TICKS,
    .deadzone_pwm = MOTOR_DEFAULT_DEADZONE_PWM,
    .pwm_limit = MOTOR_DEFAULT_PWM_LIMIT,
    .integral_limit = MOTOR_DEFAULT_INTEGRAL_LIMIT,
    .kp = 10.0f,
    .ki = 2.0f,
    .kd = 1.0f,
};

motor_t *Motor_Board_GetDefault(void)
{
    return &g_default_motor;
}

void Motor_Board_Init(void)
{
    Motor_InitDevice(&g_default_motor, &g_default_motor_cfg);
    Motor_StartDevice(&g_default_motor);
}
