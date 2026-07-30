/* SPDX-License-Identifier: MIT */
/**
 * @file    timebase.h
 * @brief   工程统一时间基准与延时接口。
 * @author  weixin
 *
 * 毫秒时间复用 HAL 的 tick（当前工程由 TIM14 提供）；微秒时间戳只是毫秒
 * tick 的单位换算，分辨率仍为 1 ms。delay_us() 则使用 Cortex-M DWT 周期计数器，
 * 适用于短时间忙等。
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief  初始化 DWT 周期计数器。
 * @return 无返回值。
 * @note   可重复调用，应在 HAL_Init() 之后调用。
 */
void timebase_init(void);

/**
 * @brief  获取当前毫秒时间戳。
 * @return 当前 HAL tick，单位 ms。
 */
uint32_t time_get_ms(void);

/**
 * @brief  获取当前微秒时间戳估算值。
 * @return 当前时间戳，单位 us。
 * @note   实际分辨率为 1 ms。
 */
uint32_t time_get_us(void);

/**
 * @brief  计算从起始毫秒时间到现在的差值。
 * @param  start_ms 起始毫秒时间戳。
 * @return 经过时间，单位 ms。
 */
uint32_t time_elapsed_ms(uint32_t start_ms);

/**
 * @brief  计算从起始微秒时间到现在的差值。
 * @param  start_us 起始微秒时间戳。
 * @return 经过时间，单位 us。
 */
uint32_t time_elapsed_us(uint32_t start_us);

/**
 * @brief  判断毫秒级超时是否到达。
 * @param  start_ms 起始毫秒时间戳。
 * @param  timeout_ms 超时时间，单位 ms。
 * @return true 表示已超时，false 表示未超时。
 */
bool time_is_timeout_ms(uint32_t start_ms, uint32_t timeout_ms);

/**
 * @brief  判断微秒级超时是否到达。
 * @param  start_us 起始微秒时间戳。
 * @param  timeout_us 超时时间，单位 us。
 * @return true 表示已超时，false 表示未超时。
 */
bool time_is_timeout_us(uint32_t start_us, uint32_t timeout_us);

/**
 * @brief  毫秒级延时。
 * @param  ms 延时时间，单位 ms。
 * @return 无返回值。
 * @note   任务调度器运行时让出 CPU，启动前使用 HAL 忙等。
 */
void delay_ms(uint32_t ms);

/**
 * @brief  微秒级忙等延时。
 * @param  us 延时时间，单位 us。
 * @return 无返回值。
 * @note   该函数会占用 CPU，不能用于长延时。
 */
void delay_us(uint32_t us);

#endif /* TIMEBASE_H */
