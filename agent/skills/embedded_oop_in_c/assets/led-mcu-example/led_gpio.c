#include "led_gpio.h"

#include <stddef.h>

#include "container_of.h"

static enum status led_gpio_write(struct led *base, bool on)
{
    struct led_gpio *self = container_of(base, struct led_gpio, base);
    GPIO_PinState state;

    state = (on == self->active_high) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(self->port, self->pin, state);
    return STATUS_OK;
}

static enum status led_gpio_on(struct led *base)
{
    return led_gpio_write(base, true);
}

static enum status led_gpio_off(struct led *base)
{
    return led_gpio_write(base, false);
}

static const struct led_ops led_gpio_ops = {
    .on = led_gpio_on,
    .off = led_gpio_off,
};

enum status led_gpio_init(struct led_gpio *self, GPIO_TypeDef *port,
                          uint16_t pin, bool active_high)
{
    enum status status;

    if ((self == NULL) || (port == NULL) || (pin == 0U)) {
        return STATUS_ERR_INVALID_ARG;
    }

    self->port = port;
    self->pin = pin;
    self->active_high = active_high;
    status = led_init(&self->base, &led_gpio_ops);
    if (status != STATUS_OK) {
        return status;
    }
    return led_off(&self->base);
}
