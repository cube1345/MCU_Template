/**
 * @file motor_board.c
 * @brief MSPM0G3507 PWM binding for the default DC motor.
 * @details Owns TimerG instances, SysConfig channel indices, and output
 *          polarity. The Device layer receives only the narrow PWM port.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#include "motor_board.h"

#include "../0_hal/hal_timer.h"
#include "ti_msp_dl_config.h"

#define MOTOR_BOARD_PWM_PERIOD_TICKS  (8000U)

typedef struct {
    GPTIMER_Regs *timer;
    DL_TIMER_CC_INDEX cc_forward;
    DL_TIMER_CC_INDEX cc_reverse;
    uint32_t period_ticks;
    uint8_t reverse_logic;
} motor_pwm_channel_t;

static motor_t g_default_motor;

static const motor_pwm_channel_t g_motor_left_channel = {
    .timer = PWM_1_INST,
    .cc_forward = GPIO_PWM_1_C0_IDX,
    .cc_reverse = GPIO_PWM_1_C1_IDX,
    .period_ticks = MOTOR_BOARD_PWM_PERIOD_TICKS,
    .reverse_logic = 1U,
};

static const motor_pwm_channel_t g_motor_right_channel = {
    .timer = PWM_0_INST,
    .cc_forward = GPIO_PWM_0_C0_IDX,
    .cc_reverse = GPIO_PWM_0_C1_IDX,
    .period_ticks = MOTOR_BOARD_PWM_PERIOD_TICKS,
    .reverse_logic = 0U,
};

static int32_t Motor_Board_ClampPwm(int32_t pwm, uint32_t period_ticks)
{
    int32_t limit = (int32_t)period_ticks;

    if (pwm > limit) {
        return limit;
    }
    if (pwm < -limit) {
        return -limit;
    }
    return pwm;
}

static void Motor_Board_StartChannel(const void *context)
{
    const motor_pwm_channel_t *channel = (const motor_pwm_channel_t *)context;

    if ((channel != 0) && (channel->timer != 0)) {
        HAL_TimerG_StartCounter(channel->timer);
    }
}

static void Motor_Board_StopChannel(const void *context)
{
    const motor_pwm_channel_t *channel = (const motor_pwm_channel_t *)context;

    if ((channel != 0) && (channel->timer != 0)) {
        HAL_TimerG_StopCounter(channel->timer);
    }
}

/**
 * @brief Write one signed PWM command with the board-defined H-bridge polarity.
 * @note This callback runs in the caller context and is not ISR synchronized.
 */
static void Motor_Board_SetChannelPwm(const void *context, int32_t pwm)
{
    const motor_pwm_channel_t *channel = (const motor_pwm_channel_t *)context;
    uint32_t forward_compare;
    uint32_t reverse_compare;

    if ((channel == 0) || (channel->timer == 0) || (channel->period_ticks == 0U)) {
        return;
    }

    pwm = Motor_Board_ClampPwm(pwm, channel->period_ticks);
    if (channel->reverse_logic != 0U) {
        pwm = -pwm;
    }

    if (pwm >= 0) {
        forward_compare = channel->period_ticks - (uint32_t)pwm;
        reverse_compare = channel->period_ticks;
    } else {
        forward_compare = channel->period_ticks;
        reverse_compare = channel->period_ticks - (uint32_t)(-pwm);
    }

    HAL_TimerG_SetCCValue(channel->timer, forward_compare, channel->cc_forward);
    HAL_TimerG_SetCCValue(channel->timer, reverse_compare, channel->cc_reverse);
}

static const motor_cfg_t g_default_motor_cfg = {
    .left = {
        .context = &g_motor_left_channel,
        .start = Motor_Board_StartChannel,
        .stop = Motor_Board_StopChannel,
        .set_pwm = Motor_Board_SetChannelPwm,
    },
    .right = {
        .context = &g_motor_right_channel,
        .start = Motor_Board_StartChannel,
        .stop = Motor_Board_StopChannel,
        .set_pwm = Motor_Board_SetChannelPwm,
    },
    .deadzone_pwm = MOTOR_DEFAULT_DEADZONE_PWM,
    .pwm_limit = MOTOR_DEFAULT_PWM_LIMIT,
    .integral_limit = MOTOR_DEFAULT_INTEGRAL_LIMIT,
    .kp = 10.0f,
    .ki = 2.0f,
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
