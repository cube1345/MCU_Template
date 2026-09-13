#ifndef MYUSED_MYFACADES_STEPPER_FACADE_H
#define MYUSED_MYFACADES_STEPPER_FACADE_H

/**
 * @file stepper_facade.h
 * @brief 步进电机兼容门面层接口。
 */

#include "stepper_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Legacy default board exposes two logical axis IDs only. */
#define STEPPER_MOTOR_COUNT (2U)

void Stepper_Facade_Init(void);

/* Compatibility entry points use the default Board controller. Multi-instance
 * applications use Stepper_Device* / Stepper_Controller* with explicit objects. */
void Stepper_Enable(Stepper_MotorId_t motorId, bool enable);
void Stepper_EnableAll(bool enable);
void Stepper_SetEnablePolarity(Stepper_MotorId_t motorId, Stepper_EnablePolarity_t polarity);
void Stepper_SetForwardLevel(Stepper_MotorId_t motorId, bool high);
void Stepper_SetDirection(Stepper_MotorId_t motorId, Stepper_Direction_t direction);
void Stepper_ToggleDirection(Stepper_MotorId_t motorId);
bool Stepper_SetFrequency(Stepper_MotorId_t motorId, uint32_t hz);
bool Stepper_SetPulseWidthUs(Stepper_MotorId_t motorId, uint32_t us);
void Stepper_Start(Stepper_MotorId_t motorId);
void Stepper_Stop(Stepper_MotorId_t motorId);
void Stepper_StartAll(void);
void Stepper_StopAll(void);
void Stepper_ConfigSyncReverse(bool enable, uint32_t periodMs);
void Stepper_Service1ms(void);
const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t motorId);
const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void);

#ifdef __cplusplus
}
#endif

#endif /* MYUSED_MYFACADES_STEPPER_FACADE_H */
