# MCU LED Example

`assets/led-mcu-example/` is MCU-first STM32 CubeMX HAL code. It intentionally
has no host `main`, Makefile, fake backend, or demo binary.

```text
application -> board_led -> struct led *
                         -> led_gpio -> HAL_GPIO_WritePin
                         -> led_pwm  -> HAL_TIM_PWM_Start/Stop + compare register
```

HAL types are confined to the GPIO/PWM concrete drivers and the board
configuration. The application sees only `struct led *` and `led_*` APIs.

## CubeMX Integration

Configure the LED GPIO as an output and configure the PWM timer/channel in
CubeMX. After `MX_GPIO_Init()` and `MX_TIM3_Init()` (use the actual generated
timer initializer), add the following to the application startup path:

```c
static const struct board_led_config led_config = {
    .status_port = STATUS_LED_GPIO_Port,
    .status_pin = STATUS_LED_Pin,
    .status_active_high = true,
    .dimmer_timer = &htim3,
    .dimmer_channel = TIM_CHANNEL_1,
    .dimmer_period = __HAL_TIM_GET_AUTORELOAD(&htim3),
};

if (board_led_init(&led_config) != STATUS_OK) {
    Error_Handler();
}
```

The application can then obtain `board_status_led()` or `board_dimmer_led()`
and call `led_on()`, `led_off()`, or `led_set_brightness()`. GPIO LEDs reject
brightness with `STATUS_ERR_UNSUPPORTED`; PWM LEDs implement it.

Build these sources with the CubeMX-generated project. Verify the generated pin
macros, timer handle, channel, polarity, PWM period, and visible board behavior
after flashing.
