#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

#include "status.h"

struct led;

struct led_ops {
    enum status (*on)(struct led *self);
    enum status (*off)(struct led *self);
    enum status (*set_brightness)(struct led *self, uint8_t percent);
};

struct led {
    const struct led_ops *ops;
    bool initialized;
};

enum status led_init(struct led *self, const struct led_ops *ops);
enum status led_on(struct led *self);
enum status led_off(struct led *self);
enum status led_set_brightness(struct led *self, uint8_t percent);

#endif
