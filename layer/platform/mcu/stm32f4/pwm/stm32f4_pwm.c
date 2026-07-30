/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_pwm.c
 * @brief   STM32F4 TIM1、TIM2和TIM3到Platform PWM的HAL适配。
 * @author  weixin
 *
 * TIM2_CH1至TIM2_CH4使用PA5、PA1、PA2、PA3，以10kHz驱动双路
 * AT8236。TIM1四路保留为后端候选能力；TIM3当前由编码器模式占用。
 */
#include "stm32f4_pwm.h"

#include "platform/inferface/pwm/pwm.h"
#include "result.h"
#include "stm32f4.h"

#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stddef.h>

#define STM32F4_TIM1_PWM_FREQUENCY_HZ UINT32_C(10000)
#define STM32F4_TIM2_PWM_FREQUENCY_HZ UINT32_C(10000)
#define STM32F4_TIM3_COUNTER_HZ UINT32_C(1000000)
#define STM32F4_TIM3_PWM_FREQUENCY_HZ UINT32_C(1000)

struct stm32f4_pwm_channel {
  int32_t id;
  TIM_HandleTypeDef *timer;
  uint32_t hal_channel;
};

static TIM_HandleTypeDef s_tim1;
static TIM_HandleTypeDef s_tim2;
static TIM_HandleTypeDef s_tim3;
static bool s_tim1_initialized;
static bool s_tim2_initialized;
static bool s_tim3_initialized;

static const struct stm32f4_pwm_channel s_channels[] = {
  {STM32F4_PWM_TIM3_CH1, &s_tim3, TIM_CHANNEL_1},
  {STM32F4_PWM_TIM1_CH1, &s_tim1, TIM_CHANNEL_1},
  {STM32F4_PWM_TIM1_CH2, &s_tim1, TIM_CHANNEL_2},
  {STM32F4_PWM_TIM1_CH3, &s_tim1, TIM_CHANNEL_3},
  {STM32F4_PWM_TIM1_CH4, &s_tim1, TIM_CHANNEL_4},
  {STM32F4_PWM_TIM2_CH1, &s_tim2, TIM_CHANNEL_1},
  {STM32F4_PWM_TIM2_CH2, &s_tim2, TIM_CHANNEL_2},
  {STM32F4_PWM_TIM2_CH3, &s_tim2, TIM_CHANNEL_3},
  {STM32F4_PWM_TIM2_CH4, &s_tim2, TIM_CHANNEL_4},
};

/**
 * @brief  判断当前是否处于中断上下文。
 * @return true表示ISR上下文，false表示任务或启动上下文。
 */
static bool stm32f4_pwm_is_in_isr(void)
{
  return __get_IPSR() != 0U;
}

/**
 * @brief  根据平台通道编号查找STM32F4 PWM映射。
 * @param  channel Platform PWM通道编号。
 * @return 对应映射地址；通道无效时返回NULL。
 */
static const struct stm32f4_pwm_channel *stm32f4_pwm_channel_get(
    int32_t channel)
{
  size_t index;

  for (index = 0U;
       index < (sizeof(s_channels) / sizeof(s_channels[0]));
       ++index) {
    if (s_channels[index].id == channel) {
      return &s_channels[index];
    }
  }

  return NULL;
}

/**
 * @brief  初始化一个PWM输出比较通道。
 * @param  timer 已完成基础配置的HAL定时器句柄。
 * @param  channel HAL TIM_CHANNEL_*通道。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_channel_init(TIM_HandleTypeDef *timer,
                                    uint32_t channel)
{
  TIM_OC_InitTypeDef config = {0};

  config.OCMode = TIM_OCMODE_PWM1;
  config.Pulse = 0U;
  config.OCPolarity = TIM_OCPOLARITY_HIGH;
  config.OCFastMode = TIM_OCFAST_DISABLE;

  return (HAL_TIM_PWM_ConfigChannel(timer, &config, channel) == HAL_OK) ?
         PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  初始化AT8236使用的TIM1四路10kHz PWM。
 * @return PLATFORM_OK表示成功或已经初始化，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_tim1_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  uint32_t timer_clock_hz;
  int result;

  if (s_tim1_initialized) {
    return PLATFORM_OK;
  }

  timer_clock_hz = HAL_RCC_GetPCLK2Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0U) {
    timer_clock_hz *= 2U;
  }
  if ((timer_clock_hz == 0U) ||
      ((timer_clock_hz % STM32F4_TIM1_PWM_FREQUENCY_HZ) != 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  __HAL_RCC_GPIOE_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &gpio);

  __HAL_RCC_TIM1_CLK_ENABLE();
  s_tim1.Instance = TIM1;
  s_tim1.Init.Prescaler = 0U;
  s_tim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  s_tim1.Init.Period =
      (timer_clock_hz / STM32F4_TIM1_PWM_FREQUENCY_HZ) - 1U;
  s_tim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  s_tim1.Init.RepetitionCounter = 0U;
  s_tim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&s_tim1) != HAL_OK) {
    return PLATFORM_ERR_IO;
  }

  result = stm32f4_pwm_channel_init(&s_tim1, TIM_CHANNEL_1);
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim1, TIM_CHANNEL_2);
  }
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim1, TIM_CHANNEL_3);
  }
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim1, TIM_CHANNEL_4);
  }
  if (result != PLATFORM_OK) {
    return result;
  }

  s_tim1_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  初始化双路 AT8236 使用的 TIM2 四路 10 kHz PWM。
 * @return PLATFORM_OK 表示成功或已经初始化，其他 PLATFORM_* 表示失败。
 * @note   TIM2 四个通道共享计数周期，因此四路输出频率必须保持一致。
 */
static int stm32f4_pwm_tim2_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  uint32_t timer_clock_hz;
  int result;

  if (s_tim2_initialized) {
    return PLATFORM_OK;
  }

  /* TIM2 位于 APB1；当 APB1 分频不为 1 时，定时器时钟为 PCLK1 的两倍。 */
  timer_clock_hz = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
    timer_clock_hz *= 2U;
  }
  if ((timer_clock_hz == 0U) ||
      ((timer_clock_hz % STM32F4_TIM2_PWM_FREQUENCY_HZ) != 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  /* PA5/PA1/PA2/PA3 分别连接 TIM2_CH1/CH2/CH3/CH4。 */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &gpio);

  __HAL_RCC_TIM2_CLK_ENABLE();
  s_tim2.Instance = TIM2;
  s_tim2.Init.Prescaler = 0U;
  s_tim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  s_tim2.Init.Period =
      (timer_clock_hz / STM32F4_TIM2_PWM_FREQUENCY_HZ) - 1U;
  s_tim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  s_tim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&s_tim2) != HAL_OK) {
    return PLATFORM_ERR_IO;
  }

  result = stm32f4_pwm_channel_init(&s_tim2, TIM_CHANNEL_1);
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim2, TIM_CHANNEL_2);
  }
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim2, TIM_CHANNEL_3);
  }
  if (result == PLATFORM_OK) {
    result = stm32f4_pwm_channel_init(&s_tim2, TIM_CHANNEL_4);
  }
  if (result != PLATFORM_OK) {
    return result;
  }

  s_tim2_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  初始化调光LED使用的TIM3_CH1 1kHz PWM。
 * @return PLATFORM_OK表示成功或已经初始化，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_tim3_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  uint32_t timer_clock_hz;

  if (s_tim3_initialized) {
    return PLATFORM_OK;
  }

  timer_clock_hz = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
    timer_clock_hz *= 2U;
  }
  if ((timer_clock_hz < STM32F4_TIM3_COUNTER_HZ) ||
      ((timer_clock_hz % STM32F4_TIM3_COUNTER_HZ) != 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  __HAL_RCC_GPIOB_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_4;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOB, &gpio);

  __HAL_RCC_TIM3_CLK_ENABLE();
  s_tim3.Instance = TIM3;
  s_tim3.Init.Prescaler =
      (timer_clock_hz / STM32F4_TIM3_COUNTER_HZ) - 1U;
  s_tim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  s_tim3.Init.Period =
      (STM32F4_TIM3_COUNTER_HZ /
       STM32F4_TIM3_PWM_FREQUENCY_HZ) - 1U;
  s_tim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  s_tim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&s_tim3) != HAL_OK) {
    return PLATFORM_ERR_IO;
  }
  if (stm32f4_pwm_channel_init(&s_tim3, TIM_CHANNEL_1) != PLATFORM_OK) {
    return PLATFORM_ERR_IO;
  }

  s_tim3_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  确保指定通道所属定时器已经完成初始化。
 * @param  channel PWM通道映射。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_hardware_init(
    const struct stm32f4_pwm_channel *channel)
{
  if (channel->timer == &s_tim1) {
    return stm32f4_pwm_tim1_init();
  }
  if (channel->timer == &s_tim2) {
    return stm32f4_pwm_tim2_init();
  }
  if (channel->timer == &s_tim3) {
    return stm32f4_pwm_tim3_init();
  }

  return PLATFORM_ERR_INVALID_CONFIGURATION;
}

/**
 * @brief  启动指定PWM通道。
 * @param  channel Platform PWM通道编号。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_enable(int32_t channel)
{
  const struct stm32f4_pwm_channel *mapping;
  int result;

  if (stm32f4_pwm_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  mapping = stm32f4_pwm_channel_get(channel);
  if (mapping == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = stm32f4_pwm_hardware_init(mapping);
  if (result != PLATFORM_OK) {
    return result;
  }

  return (HAL_TIM_PWM_Start(mapping->timer,
                            mapping->hal_channel) == HAL_OK) ?
         PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  停止指定PWM通道。
 * @param  channel Platform PWM通道编号。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_pwm_disable(int32_t channel)
{
  const struct stm32f4_pwm_channel *mapping;
  int result;

  if (stm32f4_pwm_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  mapping = stm32f4_pwm_channel_get(channel);
  if (mapping == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = stm32f4_pwm_hardware_init(mapping);
  if (result != PLATFORM_OK) {
    return result;
  }

  return (HAL_TIM_PWM_Stop(mapping->timer,
                           mapping->hal_channel) == HAL_OK) ?
         PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  设置指定PWM通道的万分比占空比。
 * @param  channel Platform PWM通道编号。
 * @param  duty 占空比，0表示持续低，10000表示持续高。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 * @note   10000映射为ARR+1，确保AT8236慢衰减模式获得真正持续高电平。
 */
static int stm32f4_pwm_set_duty(int32_t channel, uint16_t duty)
{
  const struct stm32f4_pwm_channel *mapping;
  uint32_t period;
  uint32_t compare;
  int result;

  if (stm32f4_pwm_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  if (duty > PLATFORM_PWM_DUTY_MAX) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  mapping = stm32f4_pwm_channel_get(channel);
  if (mapping == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = stm32f4_pwm_hardware_init(mapping);
  if (result != PLATFORM_OK) {
    return result;
  }

  period = __HAL_TIM_GET_AUTORELOAD(mapping->timer) + 1U;
  compare = ((uint32_t)duty * period) /
            (uint32_t)PLATFORM_PWM_DUTY_MAX;
  __HAL_TIM_SET_COMPARE(mapping->timer,
                        mapping->hal_channel,
                        compare);
  return PLATFORM_OK;
}

static const struct pwm_ops s_ops = {
  .enable = stm32f4_pwm_enable,
  .disable = stm32f4_pwm_disable,
  .set_duty = stm32f4_pwm_set_duty,
};

/**
 * @brief  注册STM32F4 PWM后端到Platform PWM子系统。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
int stm32f4_pwm_register(void)
{
  return pwm_register(&s_ops);
}
