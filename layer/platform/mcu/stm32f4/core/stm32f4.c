/**
 * @file    stm32f4.c
 * @brief   STM32F4 平台后端注册入口。
 * @author  weixin
 */
#include "stm32f4.h"
#include "stm32f4_pin.h"
#include "stm32f4_pwm.h"

#include "stm32f4xx_hal.h"

#include <stdbool.h>

static bool s_initialized;

/**
 * @brief  初始化 STM32F4 系统时钟。
 * @return 0 表示成功，-1 表示 HAL 时钟配置失败。
 * @note   系统时钟是所有 MCU 外设的共同前提，由 MCU 子系统负责。
 */
static int stm32f4_system_clock_init(void)
{
  RCC_OscInitTypeDef oscillator = {0};
  RCC_ClkInitTypeDef clock = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  oscillator.HSIState = RCC_HSI_ON;
  oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  oscillator.PLL.PLLState = RCC_PLL_ON;
  oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  oscillator.PLL.PLLM = 8U;
  oscillator.PLL.PLLN = 168U;
  oscillator.PLL.PLLP = RCC_PLLP_DIV2;
  oscillator.PLL.PLLQ = 4U;
  if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
    return -1;
  }
  clock.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clock.APB1CLKDivider = RCC_HCLK_DIV4;
  clock.APB2CLKDivider = RCC_HCLK_DIV2;
  return (HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_5) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  初始化 STM32F4 MCU 子系统。
 * @return 0 表示成功或已经初始化，-1 表示时钟或后端注册失败。
 * @note   当前会注册 Pin 和 PWM 后端，I2C/UART 由 board 层按需注册。
 */
int stm32f4_init(void)
{
  if (s_initialized) {
    return 0;
  }
  if (stm32f4_system_clock_init() != 0) {
    return -1;
  }
  if (stm32f4_pin_register() != 0) {
    return -1;
  }
  if (stm32f4_pwm_register() != 0) {
    return -1;
  }

  s_initialized = true;
  return 0;
}
