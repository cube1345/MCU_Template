/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_encoder_counter.c
 * @brief   STM32F4 TIM2、TIM3和TIM4硬件正交编码器计数后端。
 * @author  weixin
 */
#include "stm32f4_encoder_counter.h"

#include "container_of.h"
#include "result.h"
#include "stm32f4.h"

#include "stm32f4xx_hal.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STM32F4_ENCODER_INSTANCE_COUNT UINT32_C(3)
#define STM32F4_ENCODER_FILTER_MAX UINT32_C(15)
#define STM32F4_TIM2_COUNTER_MASK UINT32_MAX
#define STM32F4_TIM3_COUNTER_MASK UINT32_C(0x0000FFFF)
#define STM32F4_TIM4_COUNTER_MASK UINT32_C(0x0000FFFF)

struct stm32f4_encoder_counter {
  struct encoder_counter base;
  TIM_HandleTypeDef timer;
  struct stm32f4_encoder_counter_config config;
  uint32_t counter_mask;
  uint32_t last_raw_count;
  int64_t accumulated_count;
  bool hardware_initialized;
};

static struct stm32f4_encoder_counter
    s_instances[STM32F4_ENCODER_INSTANCE_COUNT];

/**
 * @brief  判断当前是否处于中断上下文。
 * @return true表示ISR上下文，false表示任务或启动上下文。
 */
static bool stm32f4_encoder_is_in_isr(void)
{
  return __get_IPSR() != 0U;
}

/**
 * @brief  检查板级编码器定时器和引脚组合是否合法。
 * @param  config 待检查配置。
 * @return true表示合法，false表示后端不支持。
 */
static bool stm32f4_encoder_config_is_valid(
    const struct stm32f4_encoder_counter_config *config)
{
  if ((config == NULL) || (config->name == NULL) ||
      (config->input_filter > STM32F4_ENCODER_FILTER_MAX)) {
    return false;
  }
  if (config->id == STM32F4_ENCODER_TIM2) {
    return (config->pin_a == STM32F4_PIN_PA15) &&
           (config->pin_b == STM32F4_PIN_PB3);
  }
  if (config->id == STM32F4_ENCODER_TIM4) {
    return (config->pin_a == STM32F4_PIN_PD12) &&
           (config->pin_b == STM32F4_PIN_PD13);
  }
  if (config->id == STM32F4_ENCODER_TIM3) {
    return (config->pin_a == STM32F4_PIN_PA6) &&
           (config->pin_b == STM32F4_PIN_PA7);
  }

  return false;
}

/**
 * @brief  配置TIM2编码器模式使用的PA15和PB3。
 * @return 无返回值。
 */
static void stm32f4_encoder_tim2_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF1_TIM2;

  gpio.Pin = GPIO_PIN_15;
  HAL_GPIO_Init(GPIOA, &gpio);
  gpio.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOB, &gpio);
}

/**
 * @brief  配置TIM4编码器模式使用的PD12和PD13。
 * @return 无返回值。
 */
static void stm32f4_encoder_tim4_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOD_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOD, &gpio);
}

/**
 * @brief  配置TIM3编码器模式使用的PA6和PA7。
 * @retval None。
 * @note   TIM3进入编码器模式后不能再用于PB4上的TIM3_CH1调光PWM。
 */
static void stm32f4_encoder_tim3_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOA, &gpio);
}

/**
 * @brief  选择并开启编码器定时器资源。
 * @param  self STM32F4编码器计数器对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示配置错误。
 */
static int stm32f4_encoder_timer_select(
    struct stm32f4_encoder_counter *self)
{
  if (self->config.id == STM32F4_ENCODER_TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    stm32f4_encoder_tim2_gpio_init();
    self->timer.Instance = TIM2;
    self->counter_mask = STM32F4_TIM2_COUNTER_MASK;
    return PLATFORM_OK;
  }
  if (self->config.id == STM32F4_ENCODER_TIM4) {
    __HAL_RCC_TIM4_CLK_ENABLE();
    stm32f4_encoder_tim4_gpio_init();
    self->timer.Instance = TIM4;
    self->counter_mask = STM32F4_TIM4_COUNTER_MASK;
    return PLATFORM_OK;
  }
  if (self->config.id == STM32F4_ENCODER_TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    stm32f4_encoder_tim3_gpio_init();
    self->timer.Instance = TIM3;
    self->counter_mask = STM32F4_TIM3_COUNTER_MASK;
    return PLATFORM_OK;
  }

  return PLATFORM_ERR_INVALID_CONFIGURATION;
}

/**
 * @brief  初始化定时器正交编码器接口。
 * @param  self STM32F4编码器计数器对象。
 * @return PLATFORM_OK表示成功或已初始化，其他PLATFORM_*表示失败。
 */
static int stm32f4_encoder_hardware_init(
    struct stm32f4_encoder_counter *self)
{
  TIM_Encoder_InitTypeDef encoder = {0};
  int result;

  if (self->hardware_initialized) {
    return PLATFORM_OK;
  }
  result = stm32f4_encoder_timer_select(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  self->timer.Init.Prescaler = 0U;
  self->timer.Init.CounterMode = TIM_COUNTERMODE_UP;
  self->timer.Init.Period = self->counter_mask;
  self->timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  self->timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  encoder.EncoderMode = TIM_ENCODERMODE_TI12;
  encoder.IC1Polarity = TIM_ICPOLARITY_RISING;
  encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  encoder.IC1Prescaler = TIM_ICPSC_DIV1;
  encoder.IC1Filter = self->config.input_filter;
  encoder.IC2Polarity = TIM_ICPOLARITY_RISING;
  encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  encoder.IC2Prescaler = TIM_ICPSC_DIV1;
  encoder.IC2Filter = self->config.input_filter;
  if (HAL_TIM_Encoder_Init(&self->timer, &encoder) != HAL_OK) {
    return PLATFORM_ERR_IO;
  }

  __HAL_TIM_SET_COUNTER(&self->timer, 0U);
  self->last_raw_count = 0U;
  self->accumulated_count = 0;
  self->hardware_initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  启动STM32F4硬件编码器计数。
 * @param  base Platform编码器计数器基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_encoder_start(struct encoder_counter *base)
{
  struct stm32f4_encoder_counter *self =
      container_of(base, struct stm32f4_encoder_counter, base);
  int result;

  if (stm32f4_encoder_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  result = stm32f4_encoder_hardware_init(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  self->last_raw_count =
      __HAL_TIM_GET_COUNTER(&self->timer) & self->counter_mask;

  return (HAL_TIM_Encoder_Start(&self->timer,
                                TIM_CHANNEL_ALL) == HAL_OK) ?
         PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  停止STM32F4硬件编码器计数。
 * @param  base Platform编码器计数器基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_encoder_stop(struct encoder_counter *base)
{
  struct stm32f4_encoder_counter *self =
      container_of(base, struct stm32f4_encoder_counter, base);

  if (stm32f4_encoder_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  return (HAL_TIM_Encoder_Stop(&self->timer,
                               TIM_CHANNEL_ALL) == HAL_OK) ?
         PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  将硬件回绕差值累加为有符号软件计数。
 * @param  self STM32F4编码器计数器对象。
 * @param  current 当前硬件计数值。
 * @return 无返回值。
 * @note   两次读取之间的真实变化量必须小于硬件模数的一半。
 */
static void stm32f4_encoder_accumulate(
    struct stm32f4_encoder_counter *self,
    uint32_t current)
{
  uint64_t modulus;
  uint32_t difference;
  int64_t delta;
  int64_t next;

  modulus = (uint64_t)self->counter_mask + UINT64_C(1);
  difference = (current - self->last_raw_count) &
               self->counter_mask;
  if ((uint64_t)difference <= (modulus / UINT64_C(2))) {
    delta = (int64_t)difference;
  } else {
    delta = (int64_t)((uint64_t)difference - modulus);
  }

  next = self->accumulated_count + delta;
  if (next > INT32_MAX) {
    next = INT32_MAX;
  } else if (next < INT32_MIN) {
    next = INT32_MIN;
  }
  self->accumulated_count = next;
  self->last_raw_count = current;
}

/**
 * @brief  读取STM32F4编码器累计计数。
 * @param  base Platform编码器计数器基础对象。
 * @param  count 用于接收累计计数的变量。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_encoder_read(struct encoder_counter *base,
                                int32_t *count)
{
  struct stm32f4_encoder_counter *self =
      container_of(base, struct stm32f4_encoder_counter, base);
  uint32_t current;

  if (stm32f4_encoder_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  current = __HAL_TIM_GET_COUNTER(&self->timer) &
            self->counter_mask;
  stm32f4_encoder_accumulate(self, current);
  *count = (int32_t)self->accumulated_count;
  return PLATFORM_OK;
}

/**
 * @brief  清零STM32F4编码器硬件和软件累计计数。
 * @param  base Platform编码器计数器基础对象。
 * @return PLATFORM_OK表示成功，其他PLATFORM_*表示失败。
 */
static int stm32f4_encoder_reset(struct encoder_counter *base)
{
  struct stm32f4_encoder_counter *self =
      container_of(base, struct stm32f4_encoder_counter, base);

  if (stm32f4_encoder_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  __HAL_TIM_SET_COUNTER(&self->timer, 0U);
  self->last_raw_count = 0U;
  self->accumulated_count = 0;
  return PLATFORM_OK;
}

static const struct encoder_counter_ops s_ops = {
  .start = stm32f4_encoder_start,
  .stop = stm32f4_encoder_stop,
  .read = stm32f4_encoder_read,
  .reset = stm32f4_encoder_reset,
};

/**
 * @brief  判断已绑定对象与请求配置是否相同。
 * @param  self 已绑定的STM32F4计数器对象。
 * @param  config 新请求配置。
 * @return true表示相同，false表示存在资源冲突。
 */
static bool stm32f4_encoder_config_matches(
    const struct stm32f4_encoder_counter *self,
    const struct stm32f4_encoder_counter_config *config)
{
  return (self->config.id == config->id) &&
         (self->config.pin_a == config->pin_a) &&
         (self->config.pin_b == config->pin_b) &&
         (self->config.input_filter == config->input_filter);
}

/**
 * @brief  创建或获取一个静态STM32F4编码器计数器对象。
 * @param  config 定时器编号、A/B相引脚和数字滤波配置。
 * @return 成功时返回Platform计数器对象，配置无效时返回NULL。
 */
struct encoder_counter *stm32f4_encoder_counter_create(
    const struct stm32f4_encoder_counter_config *config)
{
  struct stm32f4_encoder_counter *self;
  int result;

  if (!stm32f4_encoder_config_is_valid(config)) {
    return NULL;
  }
  self = &s_instances[config->id];
  if (self->base.ops != NULL) {
    return stm32f4_encoder_config_matches(self, config) ?
           &self->base : NULL;
  }

  self->config = *config;
  result = encoder_counter_bind(&self->base,
                                config->name,
                                &s_ops);
  return (result == PLATFORM_OK) ? &self->base : NULL;
}
