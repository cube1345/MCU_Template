#ifndef STEPPER_BOARD_H
#define STEPPER_BOARD_H

#include "stepper_driver.h"

void Stepper_Board_Init(void);
/** @brief Board-owned default controller, unbound in the template profile. */
stepper_controller_t *Stepper_Board_GetController(void);
stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId);

#endif /* STEPPER_BOARD_H */
