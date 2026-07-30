#ifndef BOARD_LED_H
#define BOARD_LED_H

#include <stdbool.h>
#include <stdint.h>

#include "led.h"
#include "main.h"

struct board_led_config {
    GPIO_TypeDef *status_port;
    uint16_t status_pin;
    bool status_active_high;
    TIM_HandleTypeDef *dimmer_timer;
    uint32_t dimmer_channel;
    uint32_t dimmer_period;
};

enum status board_led_init(const struct board_led_config *config);
struct led *board_status_led(void);
struct led *board_dimmer_led(void);

#endif
