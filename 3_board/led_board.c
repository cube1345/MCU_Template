#include "led_board.h"
#include "main.h"

static led_t g_pc13_led;
static const led_cfg_t g_pc13_led_cfg = {
    .hw =
        {
            .gpio =
                {
                    .port = GPIOC,
                    .pin = GPIO_PIN_13,
                },
        },
    .active_low = 0U,
    .default_on = 0U,
};

led_t *LED_Board_GetBlue(void) { return 0; }

led_t *LED_Board_GetRed(void) { return 0; }

led_t *LED_Board_GetTest(void) { return &g_pc13_led; }

void LED_Board_Init(void) {
  LED_Init(&g_pc13_led, &g_pc13_led_cfg);
}
