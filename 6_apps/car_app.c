#include "car_app.h"

static int g_car_target_speed_a = 0;
static int g_car_target_speed_b = 0;

void Car_Run(int speed, float bias)
{
    Car_SetTargetSpeed((int)((float)speed * bias),
                       (int)((float)speed * (2.0f - bias)));
}

void Car_SetTargetSpeed(int speedA, int speedB)
{
    g_car_target_speed_a = speedA;
    g_car_target_speed_b = speedB;
}

int Car_GetTargetSpeedA(void)
{
    return g_car_target_speed_a;
}

int Car_GetTargetSpeedB(void)
{
    return g_car_target_speed_b;
}
