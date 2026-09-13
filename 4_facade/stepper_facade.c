/** @file stepper_facade.c
 * @brief Compatibility calls resolve the Board-owned default controller.
 */
#include "stepper_facade.h"
#include "stepper_board.h"

void Stepper_Facade_Init(void) { Stepper_Board_Init(); }
void Stepper_Enable(Stepper_MotorId_t id, bool enable) { Stepper_DeviceEnable(Stepper_Board_GetDevice(id), enable); }
void Stepper_EnableAll(bool enable) { Stepper_ControllerEnableAll(Stepper_Board_GetController(), enable); }
void Stepper_SetEnablePolarity(Stepper_MotorId_t id, Stepper_EnablePolarity_t polarity) { Stepper_DeviceSetEnablePolarity(Stepper_Board_GetDevice(id), polarity); }
void Stepper_SetForwardLevel(Stepper_MotorId_t id, bool high) { Stepper_DeviceSetForwardLevel(Stepper_Board_GetDevice(id), high); }
void Stepper_SetDirection(Stepper_MotorId_t id, Stepper_Direction_t direction) { Stepper_DeviceSetDirection(Stepper_Board_GetDevice(id), direction); }
void Stepper_ToggleDirection(Stepper_MotorId_t id) { Stepper_DeviceToggleDirection(Stepper_Board_GetDevice(id)); }
bool Stepper_SetFrequency(Stepper_MotorId_t id, uint32_t hz) { return Stepper_DeviceSetFrequency(Stepper_Board_GetDevice(id), hz); }
bool Stepper_SetPulseWidthUs(Stepper_MotorId_t id, uint32_t us) { return Stepper_DeviceSetPulseWidthUs(Stepper_Board_GetDevice(id), us); }
void Stepper_Start(Stepper_MotorId_t id) { Stepper_DeviceStart(Stepper_Board_GetDevice(id)); }
void Stepper_Stop(Stepper_MotorId_t id) { Stepper_DeviceStop(Stepper_Board_GetDevice(id)); }
void Stepper_StartAll(void) { Stepper_ControllerStartAll(Stepper_Board_GetController()); }
void Stepper_StopAll(void) { Stepper_ControllerStopAll(Stepper_Board_GetController()); }
void Stepper_ConfigSyncReverse(bool enable, uint32_t ms) { Stepper_ControllerConfigSyncReverse(Stepper_Board_GetController(), enable, ms); }
void Stepper_Service1ms(void) { Stepper_ControllerService1ms(Stepper_Board_GetController()); }
const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t id) { return Stepper_DeviceGetState(Stepper_Board_GetDevice(id)); }
const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void) { return Stepper_ControllerGetSyncReverseState(Stepper_Board_GetController()); }
