#ifndef MYUSED_MYFACADES_MOTOR_FACADE_H
#define MYUSED_MYFACADES_MOTOR_FACADE_H

/**
 * @file motor_facade.h
 * @brief 直流电机兼容门面层接口。
 */

#include "../2_device/motor_driver.h"

void Motor_Init(void);
int Velocity_A(int TargetVelocity, int CurrentVelocity);
int Velocity_B(int TargetVelocity, int CurrentVelocity);
void Set_PWM(int pwma, int pwmb);

#endif /* MYUSED_MYFACADES_MOTOR_FACADE_H */
