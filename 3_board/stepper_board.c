#include "stepper_board.h"

/* This is the only compatibility singleton; Device owns no mutable globals. */
static stepper_controller_t default_controller;

void Stepper_Board_Init(void)
{
    /* No STEP/DIR/EN or timers are assigned in this board profile.
     * A concrete profile initializes its arrays through Stepper_ControllerInit,
     * then configures the control timer IRQ here, never inside Device. */
}

stepper_controller_t *Stepper_Board_GetController(void)
{
    return &default_controller;
}

stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId)
{
    return Stepper_ControllerGetDevice(&default_controller, (uint32_t)motorId);
}
