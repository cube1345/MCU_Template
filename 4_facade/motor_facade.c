#include "motor_facade.h"
#include "../3_board/motor_board.h"

void Motor_Init(void)
{
    Motor_Board_Init();
}

int Velocity_A(int TargetVelocity, int CurrentVelocity)
{
    return (int)Motor_VelocityCalc(Motor_Board_GetDefault(),
                                   MOTOR_CHANNEL_A,
                                   TargetVelocity,
                                   CurrentVelocity);
}

int Velocity_B(int TargetVelocity, int CurrentVelocity)
{
    return (int)Motor_VelocityCalc(Motor_Board_GetDefault(),
                                   MOTOR_CHANNEL_B,
                                   TargetVelocity,
                                   CurrentVelocity);
}

void Set_PWM(int pwma, int pwmb)
{
    Motor_SetPwm(Motor_Board_GetDefault(), pwma, pwmb);
}
