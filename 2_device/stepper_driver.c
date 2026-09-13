/**
 * @file stepper_driver.c
 * @brief Instance-owned stepper state and controller scheduling.
 * @details Board owns hardware allocation, timer IRQ setup and scheduling.
 * @author cube
 * @date 2026-04-11
 * @note Updated: 2026-09-14
 */
#include "stepper_driver.h"
#include <string.h>

/** @brief Reject stale handles, copied objects and uninitialized objects. */
static bool Stepper_Valid(const stepper_t *me)
{
    return me && me->base.is_init && me->controller &&
           me->controller->base.is_init && me->controller->devices &&
           me->index < me->controller->count &&
           &me->controller->devices[me->index] == me;
}

static void Stepper_ApplyEnable(stepper_t *me)
{
    bool high = (me->cfg.enPolarity == STEPPER_ENABLE_ACTIVE_HIGH) ?
                me->state.enabled : !me->state.enabled;
    Platform_GPIO_Write(&me->cfg.en, high ? 1U : 0U);
}

static void Stepper_ApplyDirection(stepper_t *me)
{
    bool high = me->state.direction == STEPPER_DIR_FORWARD ?
                me->cfg.forwardLevelHigh : !me->cfg.forwardLevelHigh;
    Platform_GPIO_Write(&me->cfg.dir, high ? 1U : 0U);
}

static void Stepper_ApplyPwm(stepper_t *me)
{
    HAL_TimerG_SetLoadValue(me->cfg.pwmInst, me->state.periodTicks - 1U);
    HAL_TimerG_SetCCValue(me->cfg.pwmInst,
                         me->state.running ? me->state.pulseTicks : 0U,
                         me->cfg.pwmCcIndex);
}

/** @brief Calculate in locals so failed updates never change live timing. */
static bool Stepper_Timing(stepper_t *me, uint32_t hz, uint32_t width)
{
    if (!Stepper_Valid(me) || hz == 0U || width == 0U) return false;
    uint32_t period = me->cfg.pwmClockHz / hz;
    if (period < 2U || period - 1U > HAL_Timer_MaxReload(me->cfg.pwmInst)) return false;
    uint64_t pulse = ((uint64_t)me->cfg.pwmClockHz * width + 999999ULL) / 1000000ULL;
    if (pulse == 0U) pulse = 1U;
    if (pulse >= period) return false;
    me->state.frequencyHz = hz;
    me->state.pulseWidthUs = width;
    me->state.periodTicks = period;
    me->state.pulseTicks = (uint32_t)pulse;
    Stepper_ApplyPwm(me);
    return true;
}

void Stepper_DeviceStop(stepper_t *me)
{
    if (!Stepper_Valid(me)) return;
    HAL_Timer_StopPWM(me->cfg.pwmInst, me->cfg.pwmCcIndex);
    me->state.running = false;
    HAL_TimerG_SetCCValue(me->cfg.pwmInst, 0U, me->cfg.pwmCcIndex);
    Platform_GPIO_Write(&me->cfg.step, 0U);
    Platform_GPIO_InitOutput(&me->cfg.step);
}

void Stepper_DeviceStart(stepper_t *me)
{
    if (!Stepper_Valid(me) || !me->state.enabled ||
        me->state.periodTicks < 2U || me->state.pulseTicks == 0U) return;
    if (me->state.running) return;
    me->state.running = true;
    Platform_GPIO_InitPeripheralOutput(&me->cfg.step, me->cfg.stepAlternate);
    Stepper_ApplyPwm(me);
    if (!HAL_Timer_StartPWM(me->cfg.pwmInst, me->cfg.pwmCcIndex)) {
        Stepper_DeviceStop(me);
    }
}

void Stepper_DeviceEnable(stepper_t *me, bool enable)
{
    if (!Stepper_Valid(me)) return;
    if (!enable) Stepper_DeviceStop(me);
    me->state.enabled = enable;
    Stepper_ApplyEnable(me);
}

void Stepper_DeviceSetDirection(stepper_t *me, Stepper_Direction_t direction)
{
    if (!Stepper_Valid(me) || (direction != STEPPER_DIR_FORWARD &&
                               direction != STEPPER_DIR_REVERSE)) return;
    bool restart = me->state.running;
    if (restart) Stepper_DeviceStop(me);
    me->state.direction = direction;
    Stepper_ApplyDirection(me);
    if (restart) Stepper_DeviceStart(me);
}

void Stepper_DeviceToggleDirection(stepper_t *me)
{
    if (!Stepper_Valid(me)) return;
    Stepper_DeviceSetDirection(me, me->state.direction == STEPPER_DIR_FORWARD ?
                                   STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD);
}

void Stepper_DeviceSetEnablePolarity(stepper_t *me, Stepper_EnablePolarity_t polarity)
{
    if (!Stepper_Valid(me) || (polarity != STEPPER_ENABLE_ACTIVE_LOW &&
                               polarity != STEPPER_ENABLE_ACTIVE_HIGH)) return;
    me->cfg.enPolarity = polarity;
    Stepper_ApplyEnable(me);
}

void Stepper_DeviceSetForwardLevel(stepper_t *me, bool high)
{
    if (!Stepper_Valid(me)) return;
    bool restart = me->state.running;
    if (restart) Stepper_DeviceStop(me);
    me->cfg.forwardLevelHigh = high;
    Stepper_ApplyDirection(me);
    if (restart) Stepper_DeviceStart(me);
}

bool Stepper_DeviceSetFrequency(stepper_t *me, uint32_t frequency_hz)
{
    return Stepper_Valid(me) && Stepper_Timing(me, frequency_hz, me->state.pulseWidthUs);
}

bool Stepper_DeviceSetPulseWidthUs(stepper_t *me, uint32_t pulse_width_us)
{
    if (!Stepper_Valid(me) || pulse_width_us == 0U) return false;
    if (me->state.frequencyHz == 0U) {
        me->state.pulseWidthUs = pulse_width_us;
        return true;
    }
    return Stepper_Timing(me, me->state.frequencyHz, pulse_width_us);
}

const Stepper_MotorState_t *Stepper_DeviceGetState(const stepper_t *me)
{
    return Stepper_Valid(me) ? &me->state : NULL;
}

/* Category adapters preserve the Core ops signatures. The controller alone
 * advances shared scheduling; per-device service deliberately does not tick it. */
static void Stepper_BaseEnable(void *me, uint8_t value) { Stepper_DeviceEnable(me, value != 0U); }
static void Stepper_BaseDirection(void *me, uint8_t value) { Stepper_DeviceSetDirection(me, (Stepper_Direction_t)value); }
static uint8_t Stepper_BaseFrequency(void *me, uint32_t value) { return Stepper_DeviceSetFrequency(me, value) ? 1U : 0U; }
static void Stepper_BaseStart(void *me) { Stepper_DeviceStart(me); }
static void Stepper_BaseStop(void *me) { Stepper_DeviceStop(me); }
static void Stepper_BaseService(void *me) { (void)me; }
static void Stepper_BasePolarity(void *me, uint8_t value) { Stepper_DeviceSetEnablePolarity(me, (Stepper_EnablePolarity_t)value); }
static void Stepper_BaseForward(void *me, uint8_t value) { Stepper_DeviceSetForwardLevel(me, value != 0U); }
static uint8_t Stepper_BasePulse(void *me, uint32_t value) { return Stepper_DeviceSetPulseWidthUs(me, value) ? 1U : 0U; }
static void Stepper_BaseToggle(void *me) { Stepper_DeviceToggleDirection(me); }
static void *Stepper_BaseState(void *me) { return (void *)Stepper_DeviceGetState(me); }

static const stepper_base_ops_t stepper_ops = {
    .enable = Stepper_BaseEnable,
    .set_direction = Stepper_BaseDirection,
    .set_frequency = Stepper_BaseFrequency,
    .start = Stepper_BaseStart,
    .stop = Stepper_BaseStop,
    .service = Stepper_BaseService,
    .set_enable_polarity = Stepper_BasePolarity,
    .set_forward_level = Stepper_BaseForward,
    .set_pulse_width_us = Stepper_BasePulse,
    .toggle_direction = Stepper_BaseToggle,
    .get_state = Stepper_BaseState,
};

bool Stepper_ControllerInit(stepper_controller_t *me, stepper_t *devices,
                            uint32_t count, const stepper_hw_cfg_t *hw_cfgs,
                            const stepper_ctrl_timer_cfg_t *ctrl_timer_cfg)
{
    if (!me || me->base.is_init || !devices || !hw_cfgs || count == 0U) return false;
    for (uint32_t i = 0U; i < count; ++i) {
        const stepper_hw_cfg_t *cfg = &hw_cfgs[i];
        if (devices[i].base.is_init || !HAL_Timer_IsPWMAvailable(cfg->pwmInst, cfg->pwmCcIndex) ||
            cfg->pwmClockHz == 0U || cfg->stepAlternate > 15U ||
            !Platform_GPIO_IsAvailable(&cfg->step) || !Platform_GPIO_IsAvailable(&cfg->dir) ||
            !Platform_GPIO_IsAvailable(&cfg->en) ||
            (cfg->enPolarity != STEPPER_ENABLE_ACTIVE_LOW &&
             cfg->enPolarity != STEPPER_ENABLE_ACTIVE_HIGH)) return false;
        /* Start/stop/load operates on the entire timer, so one timer per motor. */
        for (uint32_t j = 0U; j < i; ++j) {
            if (cfg->pwmInst->cfg.handle == hw_cfgs[j].pwmInst->cfg.handle) return false;
        }
    }
    memset(me, 0, sizeof(*me));
    me->devices = devices;
    me->count = count;
    if (ctrl_timer_cfg) me->ctrl_timer_cfg = *ctrl_timer_cfg;
    me->sync_reverse.periodMs = STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS;
    me->base.is_init = 1U;
    for (uint32_t i = 0U; i < count; ++i) {
        stepper_t *dev = &devices[i];
        memset(dev, 0, sizeof(*dev));
        dev->controller = me;
        dev->index = i;
        dev->cfg = hw_cfgs[i];
        dev->ops = &stepper_ops;
        dev->state.pulseWidthUs = STEPPER_DEFAULT_PULSE_WIDTH_US;
        dev->base.is_init = 1U;
        Stepper_DeviceStop(dev);
        Stepper_ApplyDirection(dev);
        Stepper_ApplyEnable(dev);
    }
    return true;
}

void Stepper_ControllerDeinit(stepper_controller_t *me)
{
    if (!me || !me->base.is_init) return;
    for (uint32_t i = 0U; i < me->count; ++i) {
        Stepper_DeviceEnable(&me->devices[i], false);
        memset(&me->devices[i], 0, sizeof(me->devices[i]));
    }
    memset(me, 0, sizeof(*me));
}

stepper_t *Stepper_ControllerGetDevice(stepper_controller_t *me, uint32_t index)
{
    if (!me || !me->base.is_init || !me->devices || index >= me->count) return NULL;
    stepper_t *dev = &me->devices[index];
    return Stepper_Valid(dev) && dev->controller == me ? dev : NULL;
}

void Stepper_ControllerEnableAll(stepper_controller_t *me, bool enable)
{
    if (!me || !me->base.is_init) return;
    for (uint32_t i = 0U; i < me->count; ++i) Stepper_DeviceEnable(&me->devices[i], enable);
}

void Stepper_ControllerStartAll(stepper_controller_t *me)
{
    if (!me || !me->base.is_init) return;
    for (uint32_t i = 0U; i < me->count; ++i) Stepper_DeviceStart(&me->devices[i]);
}

void Stepper_ControllerStopAll(stepper_controller_t *me)
{
    if (!me || !me->base.is_init) return;
    for (uint32_t i = 0U; i < me->count; ++i) Stepper_DeviceStop(&me->devices[i]);
}

void Stepper_ControllerConfigSyncReverse(stepper_controller_t *me, bool enable,
                                         uint32_t period_ms)
{
    if (!me || !me->base.is_init) return;
    me->sync_reverse.enabled = enable;
    me->sync_reverse.periodMs = period_ms ? period_ms : STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS;
    me->sync_reverse.elapsedMs = 0U;
}

void Stepper_ControllerService1ms(stepper_controller_t *me)
{
    if (!me || !me->base.is_init || !me->sync_reverse.enabled) return;
    if (++me->sync_reverse.elapsedMs < me->sync_reverse.periodMs) return;
    me->sync_reverse.elapsedMs = 0U;
    /* Stop every running output before changing any DIR pin. */
    for (uint32_t i = 0U; i < me->count; ++i) {
        stepper_t *dev = &me->devices[i];
        if (dev->state.running) {
            HAL_Timer_StopPWM(dev->cfg.pwmInst, dev->cfg.pwmCcIndex);
            HAL_TimerG_SetCCValue(dev->cfg.pwmInst, 0U, dev->cfg.pwmCcIndex);
        }
    }
    for (uint32_t i = 0U; i < me->count; ++i) {
        stepper_t *dev = &me->devices[i];
        dev->state.direction = dev->state.direction == STEPPER_DIR_FORWARD ?
                               STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;
        Stepper_ApplyDirection(dev);
    }
    for (uint32_t i = 0U; i < me->count; ++i) {
        if (me->devices[i].state.running) {
            me->devices[i].state.running = false;
            Stepper_DeviceStart(&me->devices[i]);
        }
    }
}

const Stepper_SyncReverse_t *Stepper_ControllerGetSyncReverseState(const stepper_controller_t *me)
{
    return me && me->base.is_init ? &me->sync_reverse : NULL;
}
