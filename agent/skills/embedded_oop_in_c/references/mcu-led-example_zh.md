# MCU LED 示例

`assets/led-mcu-example/` 是面向 MCU 的 STM32 CubeMX HAL 代码。它刻意不包含主机端 `main`、Makefile、fake 后端或演示二进制文件。

```text
application -> board_led -> struct led *
                         -> led_gpio -> HAL_GPIO_WritePin
                         -> led_pwm  -> HAL_TIM_PWM_Start/Stop + compare register
```

HAL 类型被限制在 GPIO/PWM 具体驱动和板级配置中。应用层只看到 `struct led *` 与 `led_*` API。

## CubeMX 接入

在 CubeMX 中将 LED GPIO 配置为输出，并配置 PWM 定时器/通道。在 `MX_GPIO_Init()` 和 `MX_TIM3_Init()` 之后（使用实际生成的定时器初始化函数），在应用启动路径中添加：

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

随后应用可以获取 `board_status_led()` 或 `board_dimmer_led()`，并调用 `led_on()`、`led_off()` 或 `led_set_brightness()`。GPIO LED 在设置亮度时返回 `STATUS_ERR_UNSUPPORTED`；PWM LED 实现该操作。

使用 CubeMX 生成的工程编译这些源文件。烧录后验证生成的引脚宏、定时器句柄、通道、极性、PWM 周期及板上的可见行为。
