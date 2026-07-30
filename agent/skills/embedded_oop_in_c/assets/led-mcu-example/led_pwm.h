#ifndef LED_PWM_H
#define LED_PWM_H

#include <stdint.h>

#include "led.h"
#include "main.h"

struct led_pwm {
    struct led base;
    TIM_HandleTypeDef *timer;
    uint32_t channel;
    uint32_t period;
    uint8_t brightness;
};

enum status led_pwm_init(struct led_pwm *self, TIM_HandleTypeDef *timer,
                         uint32_t channel, uint32_t period);

#endif
