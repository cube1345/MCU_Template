#include "hal_timer.h"

void HAL_TimerG_StartCounter(platform_timer_t *timer)
{
    if (timer == 0) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    (void) HAL_TIM_Base_Start((TIM_HandleTypeDef *)timer);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
#endif
}

void HAL_TimerG_StopCounter(platform_timer_t *timer)
{
    if (timer == 0) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    (void) HAL_TIM_Base_Stop((TIM_HandleTypeDef *)timer);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
#endif
}

void HAL_TimerG_SetLoadValue(platform_timer_t *timer, uint32_t value)
{
    if (timer == 0) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)timer, value);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
    (void)value;
#endif
}

void HAL_TimerG_SetCCValue(platform_timer_t *timer, uint32_t value,
                           platform_timer_channel_t channel)
{
    if (timer == 0) {
        return;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    __HAL_TIM_SET_COMPARE((TIM_HandleTypeDef *)timer, channel, value);
#else
    /* CubeMX 当前未生成 TIM HAL，保留 STM32 适配入口为空实现。 */
    (void)timer;
    (void)value;
    (void)channel;
#endif
}

hal_timer_iidx_t HAL_TimerG_GetPendingInterrupt(platform_timer_t *timer)
{
    if (timer == 0) {
        return HAL_TIMER_IIDX_NONE;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    if ((__HAL_TIM_GET_FLAG((TIM_HandleTypeDef *)timer, TIM_FLAG_UPDATE) != RESET) &&
        (__HAL_TIM_GET_IT_SOURCE((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE) != RESET)) {
        __HAL_TIM_CLEAR_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
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
    if (timer == 0) {
        return 0U;
    }

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
    return __HAL_TIM_GET_COUNTER((TIM_HandleTypeDef *)timer);
#else
    /* CubeMX 当前未生成 TIM HAL，计数值读取保留为空实现。 */
    (void)timer;
    return 0U;
#endif
}
