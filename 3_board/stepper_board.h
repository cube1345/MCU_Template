#ifndef STEPPER_BOARD_H
#define STEPPER_BOARD_H

#include "../2_device/stepper_driver.h"

void Stepper_Board_Init(void);
stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId);

#endif /* STEPPER_BOARD_H */
