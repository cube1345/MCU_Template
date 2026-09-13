/**
 * @file hal_timer.c
 * @brief Timer counter operations using injected object handles.
 * @details Peripheral clocks and initialization belong to the Board profile.
 * @author cube
 * @date 2026-09-14
 * @note Updated: 2026-09-14
 */
#include "hal_timer.h"

uint8_t HAL_Timer_IsAvailable(const platform_timer_t *timer)
{
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    return ((timer != 0) && (timer->cfg.handle != 0) &&
            (timer->cfg.handle->Instance != 0) &&
            (timer->cfg.handle->State != HAL_TIM_STATE_RESET)) ? 1U : 0U;
#else
    (void)timer;
    return 0U;
#endif
}

uint32_t HAL_Timer_MaxReload(const platform_timer_t *timer)
{
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if (HAL_Timer_IsAvailable(timer)) {
        return IS_TIM_32B_COUNTER_INSTANCE(timer->cfg.handle->Instance) ? UINT32_MAX : UINT16_MAX;
    }
#else
    (void)timer;
#endif
    return 0U;
}

uint8_t HAL_Timer_IsPWMAvailable(const platform_timer_t *timer, platform_timer_channel_t channel)
{
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    return HAL_Timer_IsAvailable(timer) &&
        (channel == TIM_CHANNEL_1 || channel == TIM_CHANNEL_2 ||
         channel == TIM_CHANNEL_3 || channel == TIM_CHANNEL_4) &&
        IS_TIM_CCX_INSTANCE(timer->cfg.handle->Instance, channel);
#else
    (void)timer; (void)channel;
    return 0U;
#endif
}

uint8_t HAL_Timer_StartPWM(platform_timer_t *timer, platform_timer_channel_t channel)
{
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if (HAL_Timer_IsPWMAvailable(timer, channel)) {
        return HAL_TIM_PWM_Start(timer->cfg.handle, channel) == HAL_OK;
    }
#else
    (void)timer; (void)channel;
#endif
    return 0U;
}

void HAL_Timer_StopPWM(platform_timer_t *timer, platform_timer_channel_t channel)
{
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if (HAL_Timer_IsPWMAvailable(timer, channel)) {
        (void)HAL_TIM_PWM_Stop(timer->cfg.handle, channel);
    }
#else
    (void)timer; (void)channel;
#endif
}

void HAL_TimerG_StartCounter(platform_timer_t *timer)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    (void) HAL_TIM_Base_Start(timer->cfg.handle);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
#endif
}

void HAL_TimerG_StopCounter(platform_timer_t *timer)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    (void) HAL_TIM_Base_Stop(timer->cfg.handle);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
#endif
}

void HAL_TimerG_SetLoadValue(platform_timer_t *timer, uint32_t value)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    __HAL_TIM_SET_AUTORELOAD(timer->cfg.handle, value);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
    (void)value;
#endif
}

void HAL_TimerG_SetCCValue(platform_timer_t *timer, uint32_t value,
                           platform_timer_channel_t channel)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if ((channel == TIM_CHANNEL_1) || (channel == TIM_CHANNEL_2) ||
        (channel == TIM_CHANNEL_3) || (channel == TIM_CHANNEL_4)) {
        __HAL_TIM_SET_COMPARE(timer->cfg.handle, channel, value);
    }
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
    (void)value;
    (void)channel;
#endif
}

hal_timer_iidx_t HAL_TimerG_GetPendingInterrupt(platform_timer_t *timer)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return HAL_TIMER_IIDX_NONE;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if ((__HAL_TIM_GET_FLAG(timer->cfg.handle, TIM_FLAG_UPDATE) != RESET) &&
        (__HAL_TIM_GET_IT_SOURCE(timer->cfg.handle, TIM_IT_UPDATE) != RESET)) {
        __HAL_TIM_CLEAR_IT(timer->cfg.handle, TIM_IT_UPDATE);
        return HAL_TIMER_IIDX_ZERO;
    }
#else
    /* CubeMX 当前未生成 TIM HAL，中断 pending 查询保留为空实现。 */
    (void)timer;
#endif

    return HAL_TIMER_IIDX_NONE;
}

hal_timer_iidx_t HAL_TimerA_GetPendingInterrupt(platform_timer_t *timer)
{
    return HAL_TimerG_GetPendingInterrupt(timer);
}

uint32_t HAL_TimerA_GetTimerCount(platform_timer_t *timer)
{
    if (HAL_Timer_IsAvailable(timer) == 0U) {
        return 0U;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    return __HAL_TIM_GET_COUNTER(timer->cfg.handle);
#else
    /* CubeMX 当前未生成 TIM HAL，计数值读取保留为空实现。 */
    (void)timer;
    return 0U;
#endif
}
