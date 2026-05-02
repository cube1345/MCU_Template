#include "hal_timer.h"

void HAL_TimerG_StartCounter(GPTIMER_Regs *timer)
{
    DL_TimerG_startCounter(timer);
}

void HAL_TimerG_StopCounter(GPTIMER_Regs *timer)
{
    DL_TimerG_stopCounter(timer);
}

void HAL_TimerG_SetLoadValue(GPTIMER_Regs *timer, uint32_t value)
{
    DL_TimerG_setLoadValue(timer, value);
}

void HAL_TimerG_SetCCValue(GPTIMER_Regs *timer, uint32_t value,
                           DL_TIMER_CC_INDEX idx)
{
    DL_TimerG_setCaptureCompareValue(timer, value, idx);
}

DL_TIMER_IIDX HAL_TimerG_GetPendingInterrupt(GPTIMER_Regs *timer)
{
    return DL_TimerG_getPendingInterrupt(timer);
}
