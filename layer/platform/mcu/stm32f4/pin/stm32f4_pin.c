/**
 * @file    stm32f4_pin.c
 * @brief   STM32F4 GPIO 到 Platform Pin 的 HAL 适配。
 * @author  weixin
 */
#include "stm32f4.h"

#include "platform/inferface/pin/pin.h"

#include "stm32f4xx_hal.h"

#include <assert.h>
#include <stdbool.h>

/**
 * @brief  将 platform 统一引脚编号转换为 HAL GPIO 端口和位掩码。
 * @param  pin platform 引脚编号。
 * @param  port 用于接收 HAL GPIO 端口指针。
 * @param  mask 用于接收 HAL GPIO 位掩码。
 * @return true 表示解码成功，false 表示参数或引脚编号无效。
 */
static bool stm32f4_pin_decode(int32_t pin, GPIO_TypeDef **port,
                               uint16_t *mask)
{
  int32_t port_id;
  int32_t pin_id;

  if ((pin < 0) || (port == NULL) || (mask == NULL)) {
    return false;
  }
  port_id = pin / STM32F4_PIN_PORT_WIDTH;
  pin_id = pin % STM32F4_PIN_PORT_WIDTH;
  if ((pin_id < 0) || (pin_id >= STM32F4_PIN_PORT_WIDTH)) {
    return false;
  }
  if (port_id == (int32_t)STM32F4_PORT_A) {
    *port = GPIOA;
    __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port_id == (int32_t)STM32F4_PORT_B) {
    *port = GPIOB;
    __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port_id == (int32_t)STM32F4_PORT_C) {
    *port = GPIOC;
    __HAL_RCC_GPIOC_CLK_ENABLE();
  } else if (port_id == (int32_t)STM32F4_PORT_D) {
    *port = GPIOD;
    __HAL_RCC_GPIOD_CLK_ENABLE();
  } else if (port_id == (int32_t)STM32F4_PORT_E) {
    *port = GPIOE;
    __HAL_RCC_GPIOE_CLK_ENABLE();
  } else {
    return false;
  }
  *mask = (uint16_t)(UINT16_C(1) << (uint32_t)pin_id);
  return true;
}

/**
 * @brief  配置 STM32F4 GPIO 引脚模式。
 * @param  pin platform 引脚编号。
 * @param  mode PIN_MODE_* 模式值。
 * @return 无返回值。
 */
static void stm32f4_pin_mode(int32_t pin, int32_t mode)
{
  GPIO_InitTypeDef config = {0};
  GPIO_TypeDef *port;
  uint16_t mask;

  assert(stm32f4_pin_decode(pin, &port, &mask));
  config.Pin = mask;
  config.Speed = GPIO_SPEED_FREQ_LOW;
  if (mode == PIN_MODE_OUTPUT) {
    HAL_GPIO_WritePin(port, mask, GPIO_PIN_RESET);
    config.Mode = GPIO_MODE_OUTPUT_PP;
    config.Pull = GPIO_NOPULL;
  } else if (mode == PIN_MODE_INPUT) {
    config.Mode = GPIO_MODE_INPUT;
    config.Pull = GPIO_NOPULL;
  } else {
    assert(mode == PIN_MODE_INPUT_PULLUP);
    config.Mode = GPIO_MODE_INPUT;
    config.Pull = GPIO_PULLUP;
  }
  HAL_GPIO_Init(port, &config);
}

/**
 * @brief  写入 STM32F4 GPIO 输出电平。
 * @param  pin platform 引脚编号。
 * @param  value PIN_LOW 或 PIN_HIGH。
 * @return 无返回值。
 */
static void stm32f4_pin_write(int32_t pin, int32_t value)
{
  GPIO_TypeDef *port;
  uint16_t mask;

  assert(stm32f4_pin_decode(pin, &port, &mask));
  assert((value == PIN_LOW) || (value == PIN_HIGH));
  HAL_GPIO_WritePin(port, mask,
                    (value == PIN_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief  读取 STM32F4 GPIO 输入电平。
 * @param  pin platform 引脚编号。
 * @return PIN_LOW 或 PIN_HIGH。
 */
static int32_t stm32f4_pin_read(int32_t pin)
{
  GPIO_TypeDef *port;
  uint16_t mask;

  assert(stm32f4_pin_decode(pin, &port, &mask));
  return (HAL_GPIO_ReadPin(port, mask) == GPIO_PIN_SET) ? PIN_HIGH : PIN_LOW;
}

/**
 * @brief  解析文本引脚名称为 platform 引脚编号。
 * @param  name 引脚名称，例如 "PA0" 或 "PA.0"。
 * @return platform 引脚编号；解析失败时返回 -1。
 */
static int32_t stm32f4_pin_get(const char *name)
{
  int32_t pin;
  int32_t port;
  uint8_t index;

  if (name == NULL) {
    return -1;
  }
  if ((name[0] != 'P') || (name[1] < 'A') || (name[1] > 'E')) {
    return -1;
  }
  index = UINT8_C(2);
  if (name[index] == '.') {
    ++index;
  }
  if ((name[index] >= '0') && (name[index] <= '9') &&
      (name[index + 1U] == '\0')) {
    pin = (int32_t)(name[index] - '0');
  } else if ((name[index] == '1') && (name[index + 1U] >= '0') &&
             (name[index + 1U] <= '5') && (name[index + 2U] == '\0')) {
    pin = INT32_C(10) + (int32_t)(name[index + 1U] - '0');
  } else {
    return -1;
  }
  port = (int32_t)(name[1] - 'A');
  return STM32F4_PIN_ID(port, pin);
}

/* MCU 后端动作表：platform 层只依赖这些抽象函数。 */
static const struct pin_ops s_ops = {
  .mode = stm32f4_pin_mode,
  .write = stm32f4_pin_write,
  .read = stm32f4_pin_read,
  .get = stm32f4_pin_get,
};

/**
 * @brief  注册 STM32F4 GPIO 后端到 platform pin 子系统。
 * @return 0 表示注册成功，-1 表示注册失败。
 */
int stm32f4_pin_register(void)
{
  return pin_register(&s_ops);
}
