/**
 * @file    stepper_board.c
 * @brief   双步进电机板级绑定层
 * @author  cube
 * @date    2026-05-03
 *
 * 职责:
 * - 初始化 EN/DIR GPIO 引脚为数字输出 (STEP 引脚由 SysConfig
 *   SYSCFG_DL_STEP_M1_PWM_init / SYSCFG_DL_STEP_M2_PWM_init 配置)
 * - 填充 stepper_hw_cfg_t[2] 和 stepper_ctrl_timer_cfg_t 并注入 Device 层
 * - 托管 STEP_CTRL_TIMER ISR (TIMG12_IRQHandler), 调用 Stepper_Service1ms()
 *
 * 所有 GPIO 操作通过 HAL 层 (HAL_GPIO_*) 完成。
 */

#include "stepper_board.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "ti_msp_dl_config.h"

/* ------------------------------------------------------------------ */
/* 板级硬件配置数组                                                     */
/* ------------------------------------------------------------------ */

static const stepper_hw_cfg_t g_stepper_hw_cfgs[STEPPER_MOTOR_COUNT] = {
    /* ---- Motor 1 (偏航角 Yaw) ---- */
    [STEPPER_MOTOR_1] = {
        .pwmInst       = STEP_M1_PWM_INST,
        .pwmClockHz    = STEP_M1_PWM_INST_CLK_FREQ,
        .pwmCcIndex    = GPIO_STEP_M1_PWM_C0_IDX,
        .step          = { GPIO_STEP_M1_PWM_C0_PORT, GPIO_STEP_M1_PWM_C0_PIN },
        .stepIomux     = GPIO_STEP_M1_PWM_C0_IOMUX,
        .stepIomuxFunc = GPIO_STEP_M1_PWM_C0_IOMUX_FUNC,
        .dir           = { STEP_DIR_PORT, STEP_DIR_M1_DIR_PIN },
        .en            = { STEP_EN_PORT, STEP_EN_M1_EN_PIN },
        .enPolarity    = STEPPER_ENABLE_ACTIVE_HIGH,
        .forwardLevelHigh = true,
    },

    /* ---- Motor 2 (俯仰角 Pitch) ---- */
    [STEPPER_MOTOR_2] = {
        .pwmInst       = STEP_M2_PWM_INST,
        .pwmClockHz    = STEP_M2_PWM_INST_CLK_FREQ,
        .pwmCcIndex    = GPIO_STEP_M2_PWM_C0_IDX,
        .step          = { GPIO_STEP_M2_PWM_C0_PORT, GPIO_STEP_M2_PWM_C0_PIN },
        .stepIomux     = GPIO_STEP_M2_PWM_C0_IOMUX,
        .stepIomuxFunc = GPIO_STEP_M2_PWM_C0_IOMUX_FUNC,
        .dir           = { STEP_DIR_PORT, STEP_DIR_M2_DIR_PIN },
        .en            = { STEP_EN_PORT, STEP_EN_M2_EN_PIN },
        .enPolarity    = STEPPER_ENABLE_ACTIVE_HIGH,
        .forwardLevelHigh = true,
    },
};

static const stepper_ctrl_timer_cfg_t g_ctrl_timer_cfg = {
    .timerInst = STEP_CTRL_TIMER_INST,
    .loadValue = STEP_CTRL_TIMER_INST_LOAD_VALUE,
    .irqNum    = STEP_CTRL_TIMER_INST_INT_IRQN,
};

static stepper_t g_stepper_devices[STEPPER_MOTOR_COUNT];

/* ------------------------------------------------------------------ */
/* 公共接口                                                           */
/* ------------------------------------------------------------------ */

void Stepper_Board_Init(void)
{
    /*
     * 1. 初始化 EN/DIR GPIO 引脚为数字输出
     *    STEP 引脚已由 SysConfig SYSCFG_DL_STEP_M1_PWM_init /
     *    SYSCFG_DL_STEP_M2_PWM_init 配置为定时器外设功能
     */
    HAL_GPIO_InitOutput(STEP_EN_M1_EN_IOMUX);
    HAL_GPIO_WritePin(STEP_EN_PORT, STEP_EN_M1_EN_PIN, 0U);

    HAL_GPIO_InitOutput(STEP_EN_M2_EN_IOMUX);
    HAL_GPIO_WritePin(STEP_EN_PORT, STEP_EN_M2_EN_PIN, 0U);

    HAL_GPIO_InitOutput(STEP_DIR_M1_DIR_IOMUX);
    HAL_GPIO_WritePin(STEP_DIR_PORT, STEP_DIR_M1_DIR_PIN, 0U);

    HAL_GPIO_InitOutput(STEP_DIR_M2_DIR_IOMUX);
    HAL_GPIO_WritePin(STEP_DIR_PORT, STEP_DIR_M2_DIR_PIN, 0U);

    /*
     * 2. 注入硬件配置到 Device 层
     *    Device 层不包含任何具体引脚号, 所有硬件信息由 Board 层注入
     */
    Stepper_Init(g_stepper_hw_cfgs, &g_ctrl_timer_cfg, g_stepper_devices);
}

stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId)
{
    if ((uint32_t)motorId >= STEPPER_MOTOR_COUNT) {
        return 0;
    }
    return &g_stepper_devices[motorId];
}

/* ------------------------------------------------------------------ */
/* 控制定时器 ISR (1ms 节拍)                                          */
/* ------------------------------------------------------------------ */

void TIMG12_IRQHandler(void)
{
    switch (HAL_TimerG_GetPendingInterrupt(STEP_CTRL_TIMER_INST)) {
    case DL_TIMERG_IIDX_ZERO:
        Stepper_Service1ms();
        break;
    default:
        break;
    }
}
