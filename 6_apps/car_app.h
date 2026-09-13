#ifndef __CAR_APP_H
#define __CAR_APP_H

#include "../4_facade/motor_facade.h"

void Car_Run(int speed, float bias);
void Car_SetTargetSpeed(int speedA, int speedB);
int Car_GetTargetSpeedA(void);
int Car_GetTargetSpeedB(void);


#endif
