#ifndef LED_GPIO_H
#define LED_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "led.h"
#include "main.h"

struct led_gpio {
    struct led base;
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_high;
};

enum status led_gpio_init(struct led_gpio *self, GPIO_TypeDef *port,
                          uint16_t pin, bool active_high);

#endif
