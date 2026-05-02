/**
 * @file    hal_timer.h
 * @brief   HAL 层 TimerG 通用接口
 * @author  cube
 * @date    2026-05-03
 *
 * 封装 DL_TimerG_* 操作，供 Device 层和 Board 层使用。
 * 禁止 Device/Board 层直接调用 DL_TimerG_* 函数。
 */

#ifndef __HAL_TIMER_H
#define __HAL_TIMER_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

void HAL_TimerG_StartCounter(GPTIMER_Regs *timer);
void HAL_TimerG_StopCounter(GPTIMER_Regs *timer);
void HAL_TimerG_SetLoadValue(GPTIMER_Regs *timer, uint32_t value);
void HAL_TimerG_SetCCValue(GPTIMER_Regs *timer, uint32_t value,
                           DL_TIMER_CC_INDEX idx);
DL_TIMER_IIDX HAL_TimerG_GetPendingInterrupt(GPTIMER_Regs *timer);

#endif /* __HAL_TIMER_H */
