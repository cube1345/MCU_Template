#include "led_pwm.h"

#include <stddef.h>

#include "container_of.h"

static enum status led_pwm_apply(struct led_pwm *self)
{
    uint32_t compare;

    compare = (uint32_t)(((uint64_t)self->period * self->brightness) / 100U);
    __HAL_TIM_SET_COMPARE(self->timer, self->channel, compare);
    return STATUS_OK;
}

static enum status led_pwm_on(struct led *base)
{
    struct led_pwm *self = container_of(base, struct led_pwm, base);

    if (HAL_TIM_PWM_Start(self->timer, self->channel) != HAL_OK) {
        return STATUS_ERR_IO;
    }
    return led_pwm_apply(self);
}

static enum status led_pwm_off(struct led *base)
{
    struct led_pwm *self = container_of(base, struct led_pwm, base);

    __HAL_TIM_SET_COMPARE(self->timer, self->channel, 0U);
    if (HAL_TIM_PWM_Stop(self->timer, self->channel) != HAL_OK) {
        return STATUS_ERR_IO;
    }
    return STATUS_OK;
}

static enum status led_pwm_set_brightness(struct led *base, uint8_t percent)
{
    struct led_pwm *self = container_of(base, struct led_pwm, base);

    self->brightness = percent;
    return led_pwm_apply(self);
}

static const struct led_ops led_pwm_ops = {
    .on = led_pwm_on,
    .off = led_pwm_off,
    .set_brightness = led_pwm_set_brightness,
};

enum status led_pwm_init(struct led_pwm *self, TIM_HandleTypeDef *timer,
                         uint32_t channel, uint32_t period)
{
    enum status status;

    if ((self == NULL) || (timer == NULL) || (period == 0U)) {
        return STATUS_ERR_INVALID_ARG;
    }

    self->timer = timer;
    self->channel = channel;
    self->period = period;
    self->brightness = 100U;
    status = led_init(&self->base, &led_pwm_ops);
    if (status != STATUS_OK) {
        return status;
    }
    return led_off(&self->base);
}
