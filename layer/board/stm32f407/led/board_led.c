/**
 * @file    board_led.c
 * @brief   STM32F407 板上 LED 的对象组合和接线绑定。
 * @author  weixin
 */
#include "board_led.h"

#include "led_gpio.h"
#include "stm32f4.h"
#include "result.h"

#include <stdbool.h>
#include <stddef.h>

/* Board 只描述接线及有效电平，不持有 GPIO/PWM 平台对象或 HAL 句柄。 */
static struct led_gpio s_red_led;
static struct led_gpio s_blue_led;
static bool s_initialized;

/**
 * @brief  初始化板级 LED 对象组合。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化或 LED 后端绑定失败。
 * @note   当前只组合红灯和蓝灯；TIM3已经分配给右编码器。
 */
int board_led_init(void)
{
  int result;

  if (s_initialized) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  if (result == 0) {
    result = led_gpio_init(&s_red_led, "red_led", STM32F4_PIN_PC5,
                           false);
  }
  if (result == 0) {
    result = led_gpio_init(&s_blue_led, "blue_led", STM32F4_PIN_PB2,
                           false);
  }
  if (result != 0) {
    return result;
  }

  s_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  获取红色 LED 抽象对象。
 * @return 初始化成功后返回 struct led *，否则返回 NULL。
 */
struct led *board_led_red_get(void)
{
  return s_initialized ? &s_red_led.base : NULL;
}

/**
 * @brief  获取蓝色 LED 抽象对象。
 * @return 初始化成功后返回 struct led *，否则返回 NULL。
 */
struct led *board_led_blue_get(void)
{
  return s_initialized ? &s_blue_led.base : NULL;
}
