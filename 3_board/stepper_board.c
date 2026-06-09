#include "stepper_board.h"

void Stepper_Board_Init(void)
{
    /* Template default: no board stepper binding. */
}

stepper_t *Stepper_Board_GetDevice(Stepper_MotorId_t motorId)
{
    (void)motorId;
    return 0;
}
