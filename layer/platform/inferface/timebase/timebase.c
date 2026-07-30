/* SPDX-License-Identifier: MIT */
/**
 * @file    timebase.c
 * @brief   基于 HAL tick、FreeRTOS 与 DWT 的统一时间基准实现。
 * @author  weixin
 */
#include "platform/inferface/timebase/timebase.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"

#include <stdbool.h>

static bool g_dwt_initialized;

/**
 * @brief  初始化 Cortex-M4 DWT 周期计数器。
 * @return 无返回值。
 * @note   可重复调用，不额外占用 TIM 外设。
 */
void timebase_init(void)
{
  if (g_dwt_initialized) {
    return;
  }

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  g_dwt_initialized = true;
}

/**
 * @brief  获取当前毫秒时间戳。
 * @return 当前 HAL tick，单位 ms。
 */
uint32_t time_get_ms(void)
{
  return HAL_GetTick();
}

/**
 * @brief  获取当前微秒时间戳估算值。
 * @return 当前时间戳，单位 us。
 * @note   时间源仍是 1 ms 分辨率的 HAL tick。
 */
uint32_t time_get_us(void)
{
  /* 保持与参考工程一致：API 单位为 us，但时间源的分辨率仍是 1 ms。 */
  return time_get_ms() * UINT32_C(1000);
}

/**
 * @brief  计算从起始毫秒时间到现在的差值。
 * @param  start_ms 起始毫秒时间戳。
 * @return 经过时间，单位 ms。
 */
uint32_t time_elapsed_ms(uint32_t start_ms)
{
  return time_get_ms() - start_ms;
}

/**
 * @brief  计算从起始微秒时间到现在的差值。
 * @param  start_us 起始微秒时间戳。
 * @return 经过时间，单位 us。
 */
uint32_t time_elapsed_us(uint32_t start_us)
{
  return time_get_us() - start_us;
}

/**
 * @brief  判断毫秒级超时是否到达。
 * @param  start_ms 起始毫秒时间戳。
 * @param  timeout_ms 超时时间，单位 ms。
 * @return true 表示已超时，false 表示未超时。
 */
bool time_is_timeout_ms(uint32_t start_ms, uint32_t timeout_ms)
{
  return time_elapsed_ms(start_ms) >= timeout_ms;
}

/**
 * @brief  判断微秒级超时是否到达。
 * @param  start_us 起始微秒时间戳。
 * @param  timeout_us 超时时间，单位 us。
 * @return true 表示已超时，false 表示未超时。
 */
bool time_is_timeout_us(uint32_t start_us, uint32_t timeout_us)
{
  return time_elapsed_us(start_us) >= timeout_us;
}

/**
 * @brief  毫秒级延时。
 * @param  ms 延时时间，单位 ms。
 * @return 无返回值。
 * @note   调度器运行后使用 vTaskDelay，否则使用 HAL_Delay。
 */
void delay_ms(uint32_t ms)
{
  if (ms == 0U) {
    return;
  }

  /* vTaskDelay 不能在 ISR 调用；ISR 内不应做毫秒级延时。 */
  if ((__get_IPSR() == 0U) &&
      (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
    vTaskDelay(pdMS_TO_TICKS(ms));
  } else {
    HAL_Delay(ms);
  }
}

/**
 * @brief  微秒级忙等延时。
 * @param  us 延时时间，单位 us。
 * @return 无返回值。
 * @note   使用 DWT 周期计数器，调用期间会占用 CPU。
 */
void delay_us(uint32_t us)
{
  uint32_t cycles_per_us;
  uint32_t start_cycle;
  uint32_t wait_cycles;

  if (us == 0U) {
    return;
  }
  timebase_init();
  cycles_per_us = SystemCoreClock / UINT32_C(1000000);
  if (cycles_per_us == 0U) {
    return;
  }

  /* 无符号减法允许 DWT 32 位周期计数器在短延时中自然回绕。 */
  wait_cycles = us * cycles_per_us;
  start_cycle = DWT->CYCCNT;
  while ((uint32_t)(DWT->CYCCNT - start_cycle) < wait_cycles) {
  }
}
