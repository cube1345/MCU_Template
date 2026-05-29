/**
 * @file stepper_driver.c
 * @brief MSPM0G3507 双步进电机底层驱动实现
 * @author cube
 * @date 2026-04-11
 *
 * 分层说明:
 * - 本文件实现步进电机运行时状态管理、PWM 参数计算、ops 回调
 * - 不包含任何 SysConfig 宏或具体引脚号
 * - 所有硬件资源由 Board 层通过 Stepper_Init() 注入
 * - GPIO 操作通过 HAL 层 (HAL_GPIO_*)
 * - 控制定时器 ISR 由 Board 层托管，本文件只提供 Stepper_Service1ms()
 */

#include "stepper_driver.h"
#include "hal_timer.h"

/* ------------------------------------------------------------------ */
/* Board 层注入的配置                                                  */
/* ------------------------------------------------------------------ */

static stepper_hw_cfg_t g_hw_cfgs[STEPPER_MOTOR_COUNT];
static stepper_ctrl_timer_cfg_t g_ctrl_timer_cfg;

/* ------------------------------------------------------------------ */
/* 运行时状态                                                          */
/* ------------------------------------------------------------------ */

static Stepper_MotorState_t g_motor_states[STEPPER_MOTOR_COUNT];
static stepper_t *g_stepper_devices;
static Stepper_SyncReverse_t g_syncReverse;

/* ------------------------------------------------------------------ */
/* 静态辅助函数前向声明                                                 */
/* ------------------------------------------------------------------ */

static Stepper_MotorState_t *Stepper_GetMotor(Stepper_MotorId_t motorId);
static stepper_t *Stepper_GetDevice(Stepper_MotorId_t motorId);
static void Stepper_ApplyEnableLevel(Stepper_MotorState_t *motor);
static void Stepper_ApplyDirectionLevel(Stepper_MotorState_t *motor);
static void Stepper_SwitchStepPinToPeripheral(Stepper_MotorState_t *motor);
static void Stepper_SwitchStepPinToGpioLow(Stepper_MotorState_t *motor);
static void Stepper_ApplyPwmRegisters(Stepper_MotorState_t *motor);
static void Stepper_StopOutputNow(Stepper_MotorState_t *motor);
static bool Stepper_RecalculateTiming(Stepper_MotorState_t *motor);
static void Stepper_ToggleDirectionBoth(void);

/* ------------------------------------------------------------------ */
/* ops 回调                                                            */
/* ------------------------------------------------------------------ */

static void Stepper_Base_Enable(void *me, uint8_t enable)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return;
    }
    motor->enabled = (enable != 0U);
    Stepper_ApplyEnableLevel(motor);
}

static void Stepper_Base_SetDirection(void *me, uint8_t direction)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    bool restartAfterChange;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return;
    }
    restartAfterChange = motor->running;
    if (restartAfterChange) {
        Stepper_StopOutputNow(motor);
    }
    motor->direction = (Stepper_Direction_t)direction;
    Stepper_ApplyDirectionLevel(motor);
    if (restartAfterChange) {
        motor->running = true;
        Stepper_SwitchStepPinToPeripheral(motor);
        Stepper_ApplyPwmRegisters(motor);
        HAL_TimerG_StartCounter(g_hw_cfgs[dev->motor_id].pwmInst);
    }
}

static uint8_t Stepper_Base_SetFrequency(void *me, uint32_t frequency_hz)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    uint32_t previousFrequencyHz;
    bool result;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return 0U;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return 0U;
    }
    previousFrequencyHz = motor->frequencyHz;
    motor->frequencyHz = frequency_hz;
    result = Stepper_RecalculateTiming(motor);
    if (!result) {
        motor->frequencyHz = previousFrequencyHz;
    }
    return result ? 1U : 0U;
}

static void Stepper_Base_Start(void *me)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return;
    }
    motor->running = true;
    Stepper_SwitchStepPinToPeripheral(motor);
    Stepper_ApplyPwmRegisters(motor);
    HAL_TimerG_StartCounter(g_hw_cfgs[dev->motor_id].pwmInst);
}

static void Stepper_Base_Stop(void *me)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return;
    }
    Stepper_StopOutputNow(motor);
}

static void Stepper_Base_Service(void *me)
{
    (void)me;
    Stepper_Service1ms();
}

static void Stepper_Base_SetEnablePolarity(void *me, uint8_t polarity)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    uint32_t idx = (uint32_t)dev->motor_id;
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    g_hw_cfgs[idx].enPolarity = (Stepper_EnablePolarity_t)polarity;
    motor = &g_motor_states[idx];
    Stepper_ApplyEnableLevel(motor);
}

static void Stepper_Base_SetForwardLevel(void *me, uint8_t forward_level_high)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    uint32_t idx = (uint32_t)dev->motor_id;
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    g_hw_cfgs[idx].forwardLevelHigh = (forward_level_high != 0U);
    motor = &g_motor_states[idx];
    Stepper_ApplyDirectionLevel(motor);
}

static uint8_t Stepper_Base_SetPulseWidthUs(void *me, uint32_t pulse_width_us)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return 0U;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return 0U;
    }
    uint32_t previous = motor->pulseWidthUs;
    motor->pulseWidthUs = pulse_width_us;
    if (!Stepper_RecalculateTiming(motor)) {
        motor->pulseWidthUs = previous;
        return 0U;
    }
    return 1U;
}

static void Stepper_Base_ToggleDirection(void *me)
{
    stepper_t *dev = (stepper_t *)me;
    Stepper_MotorState_t *motor;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return;
    }
    motor = Stepper_GetMotor(dev->motor_id);
    if (motor == 0) {
        return;
    }
    Stepper_Direction_t newDir = (motor->direction == STEPPER_DIR_FORWARD) ?
        STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;
    Stepper_Base_SetDirection(me, (uint8_t)newDir);
}

static void *Stepper_Base_GetState(void *me)
{
    stepper_t *dev = (stepper_t *)me;
    if ((dev == 0) || (dev->base.is_init == 0U)) {
        return 0;
    }
    return (void *)Stepper_GetMotor(dev->motor_id);
}

static const stepper_base_ops_t g_stepper_base_ops = {
    /* 核心操作 */
    .enable             = Stepper_Base_Enable,
    .set_direction      = Stepper_Base_SetDirection,
    .set_frequency      = Stepper_Base_SetFrequency,
    .start              = Stepper_Base_Start,
    .stop               = Stepper_Base_Stop,
    .service            = Stepper_Base_Service,
    /* 配置操作 */
    .set_enable_polarity = Stepper_Base_SetEnablePolarity,
    .set_forward_level   = Stepper_Base_SetForwardLevel,
    .set_pulse_width_us  = Stepper_Base_SetPulseWidthUs,
    /* 便捷操作 */
    .toggle_direction    = Stepper_Base_ToggleDirection,
    /* 状态查询 */
    .get_state           = Stepper_Base_GetState,
};

static inline const stepper_base_ops_t *Stepper_GetBaseOps(void)
{
    return &g_stepper_base_ops;
}

void Stepper_InitDevice(stepper_t *me, Stepper_MotorId_t motor_id)
{
    if ((me == 0) || ((uint32_t)motor_id >= STEPPER_MOTOR_COUNT)) {
        return;
    }
    me->ops = Stepper_GetBaseOps();
    me->motor_id = motor_id;
    me->base.is_init = 1U;
}

/* ------------------------------------------------------------------ */
/* 静态辅助函数实现                                                    */
/* ------------------------------------------------------------------ */

static Stepper_MotorState_t *Stepper_GetMotor(Stepper_MotorId_t motorId)
{
    if ((uint32_t)motorId >= STEPPER_MOTOR_COUNT) {
        return 0;
    }
    return &g_motor_states[(uint32_t)motorId];
}

static stepper_t *Stepper_GetDevice(Stepper_MotorId_t motorId)
{
    if ((g_stepper_devices == 0) || ((uint32_t)motorId >= STEPPER_MOTOR_COUNT)) {
        return 0;
    }
    return &g_stepper_devices[(uint32_t)motorId];
}

static void Stepper_ApplyEnableLevel(Stepper_MotorState_t *motor)
{
    const stepper_hw_cfg_t *hw;
    bool pinHigh;

    if (motor == 0) {
        return;
    }

    /* 通过 Stepper_MotorState_t 的索引反查 hw_cfg */
    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    hw = &g_hw_cfgs[idx];

    pinHigh = (hw->enPolarity == STEPPER_ENABLE_ACTIVE_HIGH) ?
        motor->enabled : !motor->enabled;

    HAL_GPIO_WritePin(hw->en.port, hw->en.pin, pinHigh ? 1U : 0U);
}

static void Stepper_ApplyDirectionLevel(Stepper_MotorState_t *motor)
{
    const stepper_hw_cfg_t *hw;
    bool pinHigh;

    if (motor == 0) {
        return;
    }

    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    hw = &g_hw_cfgs[idx];

    pinHigh = (motor->direction == STEPPER_DIR_FORWARD) ?
        hw->forwardLevelHigh : !hw->forwardLevelHigh;

    HAL_GPIO_WritePin(hw->dir.port, hw->dir.pin, pinHigh ? 1U : 0U);
}

static void Stepper_SwitchStepPinToPeripheral(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }
    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    const stepper_hw_cfg_t *hw = &g_hw_cfgs[idx];

    HAL_GPIO_InitPeripheralOutput(hw->step.port, hw->step.pin, hw->stepAlternate);
    HAL_GPIO_EnableOutput(hw->step.port, hw->step.pin);
}

static void Stepper_SwitchStepPinToGpioLow(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }
    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    const stepper_hw_cfg_t *hw = &g_hw_cfgs[idx];

    HAL_GPIO_InitOutput(hw->step.port, hw->step.pin);
    HAL_GPIO_WritePin(hw->step.port, hw->step.pin, 0U);
    HAL_GPIO_EnableOutput(hw->step.port, hw->step.pin);
}

static void Stepper_ApplyPwmRegisters(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }
    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    const stepper_hw_cfg_t *hw = &g_hw_cfgs[idx];

    HAL_TimerG_SetLoadValue(hw->pwmInst, motor->periodTicks);

    uint32_t compareValue = motor->running ? motor->pulseTicks : 0U;
    HAL_TimerG_SetCCValue(hw->pwmInst, compareValue,
                                     hw->pwmCcIndex);
}

static void Stepper_StopOutputNow(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }
    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return;
    }
    const stepper_hw_cfg_t *hw = &g_hw_cfgs[idx];

    HAL_TimerG_StopCounter(hw->pwmInst);
    motor->running = false;
    HAL_TimerG_SetCCValue(hw->pwmInst, 0U, hw->pwmCcIndex);
    Stepper_SwitchStepPinToGpioLow(motor);
}

static bool Stepper_RecalculateTiming(Stepper_MotorState_t *motor)
{
    if ((motor == 0) || (motor->frequencyHz == 0U) || (motor->pulseWidthUs == 0U)) {
        return false;
    }

    uint32_t idx = (uint32_t)(motor - g_motor_states);
    if (idx >= STEPPER_MOTOR_COUNT) {
        return false;
    }
    const stepper_hw_cfg_t *hw = &g_hw_cfgs[idx];

    uint32_t periodTicks = hw->pwmClockHz / motor->frequencyHz;
    if (periodTicks < 2U) {
        return false;
    }

    uint64_t pulseTicks = ((uint64_t)hw->pwmClockHz * (uint64_t)motor->pulseWidthUs
                           + 999999ULL) / 1000000ULL;
    if (pulseTicks == 0ULL) {
        pulseTicks = 1ULL;
    }
    if (pulseTicks >= (uint64_t)periodTicks) {
        pulseTicks = (uint64_t)(periodTicks / 2U);
        if (pulseTicks == 0ULL) {
            pulseTicks = 1ULL;
        }
    }

    motor->periodTicks = periodTicks;
    motor->pulseTicks = (uint32_t)pulseTicks;
    Stepper_ApplyPwmRegisters(motor);

    return true;
}

static void Stepper_ToggleDirectionBoth(void)
{
    bool restartM1 = g_motor_states[STEPPER_MOTOR_1].running;
    bool restartM2 = g_motor_states[STEPPER_MOTOR_2].running;

    if (restartM1) {
        Stepper_StopOutputNow(&g_motor_states[STEPPER_MOTOR_1]);
    }
    if (restartM2) {
        Stepper_StopOutputNow(&g_motor_states[STEPPER_MOTOR_2]);
    }

    g_motor_states[STEPPER_MOTOR_1].direction =
        (g_motor_states[STEPPER_MOTOR_1].direction == STEPPER_DIR_FORWARD) ?
        STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;
    g_motor_states[STEPPER_MOTOR_2].direction =
        (g_motor_states[STEPPER_MOTOR_2].direction == STEPPER_DIR_FORWARD) ?
        STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;

    Stepper_ApplyDirectionLevel(&g_motor_states[STEPPER_MOTOR_1]);
    Stepper_ApplyDirectionLevel(&g_motor_states[STEPPER_MOTOR_2]);

    if (restartM1) {
        Stepper_Start(STEPPER_MOTOR_1);
    }
    if (restartM2) {
        Stepper_Start(STEPPER_MOTOR_2);
    }
}

/* ------------------------------------------------------------------ */
/* 公开接口                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief 初始化步进底层驱动
 * @param hw_cfgs 双电机硬件配置数组
 * @param ctrl_timer_cfg 控制定时器配置
 *
 * Board 层在完成 GPIO IOMUX 初始化后调用。
 * 本函数:
 * 1. 存储硬件配置
 * 2. 初始化运行时状态到默认值
 * 3. 初始化设备对象并绑定 ops
 * 4. 启动控制定时器
 */
void Stepper_Init(const stepper_hw_cfg_t hw_cfgs[STEPPER_MOTOR_COUNT],
                  const stepper_ctrl_timer_cfg_t *ctrl_timer_cfg,
                  stepper_t devices[STEPPER_MOTOR_COUNT])
{
    uint32_t i;

    if ((hw_cfgs == 0) || (ctrl_timer_cfg == 0) || (devices == 0)) {
        return;
    }

    /* 存储 Board 层注入的硬件配置和设备对象指针 */
    for (i = 0U; i < STEPPER_MOTOR_COUNT; ++i) {
        g_hw_cfgs[i] = hw_cfgs[i];
    }
    g_ctrl_timer_cfg = *ctrl_timer_cfg;
    g_stepper_devices = devices;

    /* 初始化运行时状态并应用默认 GPIO 电平 */
    for (i = 0U; i < STEPPER_MOTOR_COUNT; ++i) {
        Stepper_StopOutputNow(&g_motor_states[i]);
        Stepper_ApplyPwmRegisters(&g_motor_states[i]);
        Stepper_ApplyDirectionLevel(&g_motor_states[i]);
        Stepper_ApplyEnableLevel(&g_motor_states[i]);
    }

    g_syncReverse.enabled = false;
    g_syncReverse.periodMs = STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS;
    g_syncReverse.elapsedMs = 0U;

    Stepper_InitDevice(&devices[STEPPER_MOTOR_1], STEPPER_MOTOR_1);
    Stepper_InitDevice(&devices[STEPPER_MOTOR_2], STEPPER_MOTOR_2);

    /* 启动控制定时器 */
    HAL_TimerG_StopCounter(g_ctrl_timer_cfg.timerInst);
    HAL_TimerG_SetLoadValue(g_ctrl_timer_cfg.timerInst, g_ctrl_timer_cfg.loadValue);
    HAL_TimerG_StartCounter(g_ctrl_timer_cfg.timerInst);

    NVIC_ClearPendingIRQ(g_ctrl_timer_cfg.irqNum);
    NVIC_EnableIRQ(g_ctrl_timer_cfg.irqNum);
}

void Stepper_Enable(Stepper_MotorId_t motorId, bool enable)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->enable == 0)) {
        return;
    }
    dev->ops->enable(dev, enable ? 1U : 0U);
}

void Stepper_EnableAll(bool enable)
{
    Stepper_Enable(STEPPER_MOTOR_1, enable);
    Stepper_Enable(STEPPER_MOTOR_2, enable);
}

void Stepper_SetEnablePolarity(Stepper_MotorId_t motorId,
                               Stepper_EnablePolarity_t polarity)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->set_enable_polarity == 0)) {
        return;
    }
    dev->ops->set_enable_polarity(dev, (uint8_t)polarity);
}

void Stepper_SetForwardLevel(Stepper_MotorId_t motorId, bool forwardLevelHigh)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->set_forward_level == 0)) {
        return;
    }
    dev->ops->set_forward_level(dev, forwardLevelHigh ? 1U : 0U);
}

void Stepper_SetDirection(Stepper_MotorId_t motorId,
                          Stepper_Direction_t direction)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->set_direction == 0)) {
        return;
    }
    dev->ops->set_direction(dev, (uint8_t)direction);
}

void Stepper_ToggleDirection(Stepper_MotorId_t motorId)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->toggle_direction == 0)) {
        return;
    }
    dev->ops->toggle_direction(dev);
}

bool Stepper_SetFrequency(Stepper_MotorId_t motorId, uint32_t frequencyHz)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return false;
    }
    if ((dev->ops == 0) || (dev->ops->set_frequency == 0)) {
        return false;
    }
    return dev->ops->set_frequency(dev, frequencyHz) != 0U;
}

bool Stepper_SetPulseWidthUs(Stepper_MotorId_t motorId, uint32_t pulseWidthUs)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return false;
    }
    if ((dev->ops == 0) || (dev->ops->set_pulse_width_us == 0)) {
        return false;
    }
    return dev->ops->set_pulse_width_us(dev, pulseWidthUs) != 0U;
}

void Stepper_Start(Stepper_MotorId_t motorId)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->start == 0)) {
        return;
    }
    dev->ops->start(dev);
}

void Stepper_Stop(Stepper_MotorId_t motorId)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return;
    }
    if ((dev->ops == 0) || (dev->ops->stop == 0)) {
        return;
    }
    dev->ops->stop(dev);
}

void Stepper_StartAll(void)
{
    Stepper_Start(STEPPER_MOTOR_1);
    Stepper_Start(STEPPER_MOTOR_2);
}

void Stepper_StopAll(void)
{
    Stepper_Stop(STEPPER_MOTOR_1);
    Stepper_Stop(STEPPER_MOTOR_2);
}

void Stepper_ConfigSyncReverse(bool enable, uint32_t periodMs)
{
    g_syncReverse.enabled = enable;
    g_syncReverse.periodMs = (periodMs == 0U) ?
        STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS : periodMs;
    g_syncReverse.elapsedMs = 0U;
}

void Stepper_Service1ms(void)
{
    if (!g_syncReverse.enabled) {
        return;
    }
    g_syncReverse.elapsedMs++;
    if (g_syncReverse.elapsedMs < g_syncReverse.periodMs) {
        return;
    }
    g_syncReverse.elapsedMs = 0U;
    Stepper_ToggleDirectionBoth();
}

const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t motorId)
{
    stepper_t *dev = Stepper_GetDevice(motorId);
    if (dev == 0) {
        return 0;
    }
    if ((dev->ops == 0) || (dev->ops->get_state == 0)) {
        return 0;
    }
    return (const Stepper_MotorState_t *)dev->ops->get_state(dev);
}

const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void)
{
    return &g_syncReverse;
}
