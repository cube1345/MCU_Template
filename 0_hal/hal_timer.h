/**
 * @file    hal_timer.h
 * @brief   HAL 层 TIM 通用接口
 * @author  cube
 * @date    2026-05-03
 * @note Updated: 2026-09-14
 * @details Board injects initialized handles; unavailable timers are safe no-ops.
 *
 * 封装 STM32 HAL TIM 操作，供 Device 层和 Board 层使用。
 */

#ifndef __HAL_TIMER_H
#define __HAL_TIMER_H

#include <stdint.h>
#include "stm32u5xx_hal.h"

#if defined(HAL_TIM_MODULE_ENABLED)
#include "stm32u5xx_hal_tim.h"
#define HAL_TIMER_STM32_HAS_TIM         (1U)
#endif

#ifndef HAL_TIMER_STM32_HAS_TIM
#define HAL_TIMER_STM32_HAS_TIM         (0U)
#endif

typedef enum {
    HAL_TIMER_IIDX_NONE = 0U,
    HAL_TIMER_IIDX_ZERO = 1U,
} hal_timer_iidx_t;

#if (HAL_TIMER_STM32_HAS_TIM == 1U)
typedef TIM_HandleTypeDef *platform_timer_handle_t;
#else
typedef void *platform_timer_handle_t;
#endif

/** @brief Board binding; CubeMX configures the timer and its channels first. */
typedef struct {
    platform_timer_handle_t handle;
} platform_timer_cfg_t;

/** @brief Timer object with an explicitly injected hardware handle. */
typedef struct {
    platform_timer_cfg_t cfg;
} platform_timer_t;
typedef uint32_t platform_timer_channel_t;

/** @brief Return one for a bound timer when the TIM module is enabled. */
uint8_t HAL_Timer_IsAvailable(const platform_timer_t *timer);
/** @brief Largest ARR supported by this timer; zero when unavailable. */
uint32_t HAL_Timer_MaxReload(const platform_timer_t *timer);
/** @brief Validate an available PWM channel (1..4 supported by this adapter). */
uint8_t HAL_Timer_IsPWMAvailable(const platform_timer_t *timer, platform_timer_channel_t channel);
/** @brief Start/stop a CubeMX-configured PWM channel; return one on success. */
uint8_t HAL_Timer_StartPWM(platform_timer_t *timer, platform_timer_channel_t channel);
void HAL_Timer_StopPWM(platform_timer_t *timer, platform_timer_channel_t channel);
/** @brief Start the counter; channel output and interrupt setup remain in Board. */
void HAL_TimerG_StartCounter(platform_timer_t *timer);
/** @brief Stop a bound counter; unavailable objects are a safe no-op. */
void HAL_TimerG_StopCounter(platform_timer_t *timer);
/** @brief Set ARR (period minus one) on a bound timer. */
void HAL_TimerG_SetLoadValue(platform_timer_t *timer, uint32_t value);
/** @brief Set CCR for TIM_CHANNEL_1..4; reject unsupported channel values. */
void HAL_TimerG_SetCCValue(platform_timer_t *timer, uint32_t value,
                           platform_timer_channel_t channel);
/** @brief Return and acknowledge an enabled update interrupt. */
hal_timer_iidx_t HAL_TimerG_GetPendingInterrupt(platform_timer_t *timer);

/* TimerA compatibility wrappers */
/** @brief Compatibility alias for reading/acknowledging an update interrupt. */
hal_timer_iidx_t HAL_TimerA_GetPendingInterrupt(platform_timer_t *timer);
/** @brief Read CNT from an available timer, or return zero. */
uint32_t HAL_TimerA_GetTimerCount(platform_timer_t *timer);

#endif /* __HAL_TIMER_H */
