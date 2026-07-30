#include "board_led.h"

#include <stddef.h>

#include "led_gpio.h"
#include "led_pwm.h"

static struct led_gpio status_led;
static struct led_pwm dimmer_led;
static bool board_led_initialized;

enum status board_led_init(const struct board_led_config *config)
{
    enum status status;

    if ((config == NULL) || (config->status_port == NULL) ||
        (config->status_pin == 0U) || (config->dimmer_timer == NULL) ||
        (config->dimmer_period == 0U)) {
        return STATUS_ERR_INVALID_ARG;
    }
    if (board_led_initialized) {
        return STATUS_ERR_INVALID_STATE;
    }

    status = led_gpio_init(&status_led, config->status_port, config->status_pin,
                           config->status_active_high);
    if (status != STATUS_OK) {
        return status;
    }
    status = led_pwm_init(&dimmer_led, config->dimmer_timer,
                          config->dimmer_channel, config->dimmer_period);
    if (status != STATUS_OK) {
        return status;
    }

    board_led_initialized = true;
    return STATUS_OK;
}

struct led *board_status_led(void)
{
    return board_led_initialized ? &status_led.base : NULL;
}

struct led *board_dimmer_led(void)
{
    return board_led_initialized ? &dimmer_led.base : NULL;
}
