#include "led_board.h"
#include "main.h"

static led_t g_test_led;

led_t *LED_Board_GetBlue(void) { return 0; }

led_t *LED_Board_GetRed(void) { return 0; }

led_t *LED_Board_GetTest(void) { return &g_test_led; }

void LED_Board_Init(void) {
  const led_cfg_t test_led_cfg = {
      .hw =
          {
              .gpio =
                  {
                      .port = Test_LED_GPIO_Port,
                      .pin = Test_LED_Pin,
                  },
          },
      .active_low = 0U,
      .default_on = 1U,
  };

  LED_Init(&g_test_led, &test_led_cfg);
}
