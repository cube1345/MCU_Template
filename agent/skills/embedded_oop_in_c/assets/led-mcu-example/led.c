#include "led.h"

#include <stddef.h>

enum status led_init(struct led *self, const struct led_ops *ops)
{
    if ((self == NULL) || (ops == NULL) || (ops->on == NULL) ||
        (ops->off == NULL)) {
        return STATUS_ERR_INVALID_ARG;
    }

    self->ops = ops;
    self->initialized = true;
    return STATUS_OK;
}

enum status led_on(struct led *self)
{
    if ((self == NULL) || !self->initialized || (self->ops == NULL)) {
        return STATUS_ERR_INVALID_STATE;
    }
    return self->ops->on(self);
}

enum status led_off(struct led *self)
{
    if ((self == NULL) || !self->initialized || (self->ops == NULL)) {
        return STATUS_ERR_INVALID_STATE;
    }
    return self->ops->off(self);
}

enum status led_set_brightness(struct led *self, uint8_t percent)
{
    if ((self == NULL) || !self->initialized || (self->ops == NULL)) {
        return STATUS_ERR_INVALID_STATE;
    }
    if (percent > 100U) {
        return STATUS_ERR_INVALID_ARG;
    }
    if (self->ops->set_brightness == NULL) {
        return STATUS_ERR_UNSUPPORTED;
    }
    return self->ops->set_brightness(self, percent);
}
