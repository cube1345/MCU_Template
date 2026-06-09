/**
 * @file    hal_timer.h
 * @brief   HAL 层 TIM 通用接口
 * @author  cube
 * @date    2026-05-03
 *
 * 封装 STM32 HAL TIM 操作，供 Device 层和 Board 层使用。
 */

#ifndef __HAL_TIMER_H
#define __HAL_TIMER_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#if defined(HAL_TIM_MODULE_ENABLED) && defined(__has_include)
#if __has_include("stm32f4xx_hal_tim.h")
#include "stm32f4xx_hal_tim.h"
#define HAL_TIMER_STM32_HAS_TIM         (1U)
#endif
#endif

#ifndef HAL_TIMER_STM32_HAS_TIM
#define HAL_TIMER_STM32_HAS_TIM         (0U)
#endif

typedef enum {
    HAL_TIMER_IIDX_NONE = 0U,
    HAL_TIMER_IIDX_ZERO = 1U,
} hal_timer_iidx_t;

/*
 * 当前 CubeMX 工程未启用 TIM HAL，Drivers 目录中也没有 stm32f4xx_hal_tim.h。
 * 因此这里使用不依赖具体 TIM 类型的 platform_timer_t 占位。
 * 后续启用 TIM HAL 后，platform_timer_t 会自动切换为 TIM_HandleTypeDef。
 */
#if (HAL_TIMER_STM32_HAS_TIM == 1U)
typedef TIM_HandleTypeDef platform_timer_t;
#else
typedef void platform_timer_t;
#endif
typedef uint32_t platform_timer_channel_t;

void HAL_TimerG_StartCounter(platform_timer_t *timer);
void HAL_TimerG_StopCounter(platform_timer_t *timer);
void HAL_TimerG_SetLoadValue(platform_timer_t *timer, uint32_t value);
void HAL_TimerG_SetCCValue(platform_timer_t *timer, uint32_t value,
                           platform_timer_channel_t channel);
hal_timer_iidx_t HAL_TimerG_GetPendingInterrupt(platform_timer_t *timer);

/* TimerA compatibility wrappers */
hal_timer_iidx_t HAL_TimerA_GetPendingInterrupt(platform_timer_t *timer);
uint32_t HAL_TimerA_GetTimerCount(platform_timer_t *timer);

#endif /* __HAL_TIMER_H */
