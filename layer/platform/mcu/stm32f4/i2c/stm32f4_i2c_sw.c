/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_i2c_sw.c
 * @brief   STM32F4 GPIO 模拟 I2C 后端。
 * @author  weixin
 *
 * 软件 I2C 使用 platform pin 接口驱动 SCL/SDA。释放总线时切换为上拉输入，
 * 因而需要外部上拉电阻或可靠的 MCU 内部上拉。该后端提供完整的阻塞事务，
 * 但没有定时器状态机，所以异步能力表故意为空，由公共接口返回 UNSUPPORTED。
 */
#include "stm32f4.h"
#include "stm32f4_i2c.h"
#include "container_of.h"
#include "platform/inferface/i2c/i2c.h"
#include "platform/inferface/i2c/i2c_blocking.h"
#include "platform/inferface/pin/pin.h"
#include "result.h"

#include <stddef.h>

#define STM32F4_SW_I2C_ADDR_MAX UINT16_C(0x7F)
#define STM32F4_SW_I2C_DEFAULT_HALF_PERIOD_US UINT32_C(5)

struct stm32f4_i2c_sw_bus {
  struct i2c base;
  uint32_t half_period_us;
};

/* 软件 I2C 总线实例表；每条总线使用独立的 platform 对象。 */
static struct stm32f4_i2c_sw_bus g_sw_i2c_table[] = {
  {.base = {.id = STM32F4_I2C1, .name = "i2c1-sw"}},
  {.base = {.id = STM32F4_I2C2, .name = "i2c2-sw"}},
  {.base = {.id = STM32F4_I2C3, .name = "i2c3-sw"}},
};

/**
 * @brief  软件 I2C 粗粒度微秒延时。
 * @param  us 延时时间，单位 us。
 * @return 无返回值。
 * @note   实际周期会随编译器优化和系统主频变化。
 */
static void sw_delay(uint32_t us)
{
  volatile uint32_t count = us * UINT32_C(16);
  while (count-- != 0U) { }
}

/**
 * @brief  主动拉低软件 I2C 总线引脚。
 * @param  pin platform 引脚编号。
 * @return 无返回值。
 */
static void sw_low(int32_t pin)
{
  pin_write(pin, PIN_LOW);
  pin_mode(pin, PIN_MODE_OUTPUT);
}

/**
 * @brief  释放软件 I2C 总线引脚。
 * @param  pin platform 引脚编号。
 * @return 无返回值。
 * @note   释放后由外部上拉或内部上拉将总线拉高。
 */
static void sw_release(int32_t pin)
{
  pin_mode(pin, PIN_MODE_INPUT_PULLUP);
}

/**
 * @brief  等待指定引脚变为高电平。
 * @param  pin platform 引脚编号。
 * @param  timeout_us 等待超时时间，单位 us。
 * @return PLATFORM_OK 表示变高；超时返回 PLATFORM_ERR_TIMEOUT。
 */
static int sw_wait_high(int32_t pin, uint32_t timeout_us)
{
  while (timeout_us-- != 0U) {
    if (pin_read(pin) == PIN_HIGH) return PLATFORM_OK;
    sw_delay(1U);
  }
  return PLATFORM_ERR_TIMEOUT;
}

/**
 * @brief  产生软件 I2C START 条件。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @return 无返回值。
 */
static void sw_start(const struct i2c_bus_config *config, uint32_t half)
{
  sw_release(config->sda_pin); sw_release(config->scl_pin); sw_delay(half);
  sw_low(config->sda_pin); sw_delay(half); sw_low(config->scl_pin);
}

/**
 * @brief  产生软件 I2C STOP 条件。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @return 无返回值。
 */
static void sw_stop(const struct i2c_bus_config *config, uint32_t half)
{
  sw_low(config->sda_pin); sw_delay(half); sw_release(config->scl_pin);
  (void)sw_wait_high(config->scl_pin, half + 1U); sw_delay(half);
  sw_release(config->sda_pin); sw_delay(half);
}

/**
 * @brief  写出一个软件 I2C 数据位。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @param  bit 待输出位，0 表示低电平，非 0 表示释放为高电平。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int sw_write_bit(const struct i2c_bus_config *config, uint32_t half, int bit)
{
  if (bit != 0) sw_release(config->sda_pin); else sw_low(config->sda_pin);
  sw_delay(half); sw_release(config->scl_pin);
  if (sw_wait_high(config->scl_pin, half + 1U) != PLATFORM_OK) return PLATFORM_ERR_TIMEOUT;
  sw_delay(half); sw_low(config->scl_pin); return PLATFORM_OK;
}

/**
 * @brief  读取一个软件 I2C 数据位。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @param  bit 用于接收读取位的输出参数。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int sw_read_bit(const struct i2c_bus_config *config, uint32_t half, int *bit)
{
  if (bit == NULL) return PLATFORM_ERR_INVALID_ARGUMENT;
  sw_release(config->sda_pin); sw_delay(half); sw_release(config->scl_pin);
  if (sw_wait_high(config->scl_pin, half + 1U) != PLATFORM_OK) return PLATFORM_ERR_TIMEOUT;
  *bit = (pin_read(config->sda_pin) == PIN_HIGH) ? 1 : 0;
  sw_delay(half); sw_low(config->scl_pin); return PLATFORM_OK;
}

/**
 * @brief  写出一个软件 I2C 字节并读取 ACK。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @param  value 待写出字节。
 * @return PLATFORM_OK 表示收到 ACK；其他 PLATFORM_* 表示失败。
 */
static int sw_write_byte(const struct i2c_bus_config *config, uint32_t half, uint8_t value)
{
  int bit; int result;
  for (bit = 7; bit >= 0; --bit) {
    result = sw_write_bit(config, half, (value >> (uint32_t)bit) & 1U);
    if (result != PLATFORM_OK) return result;
  }
  result = sw_read_bit(config, half, &bit);
  if (result != PLATFORM_OK) return result;
  return (bit == 0) ? PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  读取一个软件 I2C 字节并回复 ACK/NACK。
 * @param  config I2C 总线配置。
 * @param  half 半周期延时，单位 us。
 * @param  value 用于接收字节的输出参数。
 * @param  acknowledge 非 0 发送 ACK，0 发送 NACK。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int sw_read_byte(const struct i2c_bus_config *config, uint32_t half,
                        uint8_t *value, int acknowledge)
{
  int bit; int result; int index; uint8_t result_value = 0U;
  if (value == NULL) return PLATFORM_ERR_INVALID_ARGUMENT;
  for (index = 7; index >= 0; --index) {
    result = sw_read_bit(config, half, &bit);
    if (result != PLATFORM_OK) return result;
    result_value |= (uint8_t)((uint32_t)bit << (uint32_t)index);
  }
  result = sw_write_bit(config, half, acknowledge ? 0 : 1);
  if (result == PLATFORM_OK) *value = result_value;
  return result;
}

/**
 * @brief  校验软件 I2C 消息参数。
 * @param  msg 待校验的 I2C 消息。
 * @return PLATFORM_OK 表示合法；其他 PLATFORM_* 表示失败。
 */
static int sw_msg_valid(const struct i2c_msg *msg)
{
  if ((msg == NULL) || (msg->addr > STM32F4_SW_I2C_ADDR_MAX) ||
      ((msg->len > 0U) && (msg->buf == NULL))) return PLATFORM_ERR_INVALID_ARGUMENT;
  if ((msg->flags & PLATFORM_I2C_NO_START) != 0U) return PLATFORM_ERR_UNSUPPORTED;
  return PLATFORM_OK;
}

/**
 * @brief  执行一组软件 I2C 阻塞消息。
 * @param  bus I2C 基类对象。
 * @param  msgs I2C 消息数组。
 * @param  num 消息数量。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   相邻消息使用重复 START 保持总线事务连续。
 */
static int sw_transfer(struct i2c *bus, struct i2c_msg *msgs, uint32_t num)
{
  uint32_t index; uint16_t byte; int result; int read;
  struct stm32f4_i2c_sw_bus *self;
  if ((bus == NULL) || (bus->config == NULL) || (msgs == NULL) || (num == 0U))
    return PLATFORM_ERR_INVALID_ARGUMENT;
  self = container_of(bus, struct stm32f4_i2c_sw_bus, base);
  sw_start(bus->config, self->half_period_us);
  for (index = 0U; index < num; ++index) {
    if (sw_msg_valid(&msgs[index]) != PLATFORM_OK) { sw_stop(bus->config, self->half_period_us); return PLATFORM_ERR_INVALID_ARGUMENT; }
    /* 多消息事务通过重复起始条件保持总线占用，例如寄存器地址写后读取。 */
    if (index != 0U) sw_start(bus->config, self->half_period_us);
    read = ((msgs[index].flags & PLATFORM_I2C_RD) != 0U) ? 1 : 0;
    result = sw_write_byte(bus->config, self->half_period_us,
                           (uint8_t)((msgs[index].addr << 1U) | (uint16_t)read));
    if (result != PLATFORM_OK) { sw_stop(bus->config, self->half_period_us); return result; }
    for (byte = 0U; byte < msgs[index].len; ++byte) {
      if (read) result = sw_read_byte(bus->config, self->half_period_us, &msgs[index].buf[byte], byte + 1U < msgs[index].len);
      else result = sw_write_byte(bus->config, self->half_period_us, msgs[index].buf[byte]);
      if (result != PLATFORM_OK) { sw_stop(bus->config, self->half_period_us); return result; }
    }
  }
  sw_stop(bus->config, self->half_period_us);
  return PLATFORM_OK;
}

/**
 * @brief  执行软件 I2C 内存读取事务。
 * @param  bus I2C 基类对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 接收缓冲区。
 * @param  size 读取字节数。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int sw_mem_read(struct i2c *bus, const struct i2c_mem_address *address, uint8_t *data, uint16_t size)
{
  struct i2c_msg msgs[2]; uint8_t mem[2]; uint16_t count;
  if ((address == NULL) || (data == NULL) || (size == 0U) || (address->device_addr > STM32F4_SW_I2C_ADDR_MAX)) return PLATFORM_ERR_INVALID_ARGUMENT;
  count = address->memory_addr_size;
  if ((count != PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) && (count != PLATFORM_I2C_MEM_ADDR_SIZE_16BIT)) return PLATFORM_ERR_INVALID_ARGUMENT;
  if (count == 1U) mem[0] = (uint8_t)address->memory_addr;
  else { mem[0] = (uint8_t)(address->memory_addr >> 8U); mem[1] = (uint8_t)address->memory_addr; }
  msgs[0].addr = address->device_addr; msgs[0].flags = PLATFORM_I2C_WR; msgs[0].len = count; msgs[0].buf = mem;
  msgs[1].addr = address->device_addr; msgs[1].flags = PLATFORM_I2C_RD; msgs[1].len = size; msgs[1].buf = data;
  return sw_transfer(bus, msgs, 2U);
}

/**
 * @brief  执行软件 I2C 内存写入事务。
 * @param  bus I2C 基类对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 待写入数据缓冲区。
 * @param  size 写入字节数。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int sw_mem_write(struct i2c *bus, const struct i2c_mem_address *address, const uint8_t *data, uint16_t size)
{
  uint8_t buffer[2U + 255U]; struct i2c_msg msg; uint16_t count; uint16_t i;
  if ((address == NULL) || (data == NULL) || (size == 0U) || (address->device_addr > STM32F4_SW_I2C_ADDR_MAX) || (size > 255U)) return PLATFORM_ERR_INVALID_ARGUMENT;
  count = address->memory_addr_size;
  if ((count != 1U) && (count != 2U)) return PLATFORM_ERR_INVALID_ARGUMENT;
  if (count == 1U) buffer[0] = (uint8_t)address->memory_addr;
  else { buffer[0] = (uint8_t)(address->memory_addr >> 8U); buffer[1] = (uint8_t)address->memory_addr; }
  for (i = 0U; i < size; ++i) buffer[count + i] = data[i];
  msg.addr = address->device_addr; msg.flags = PLATFORM_I2C_WR; msg.len = count + size; msg.buf = buffer;
  return sw_transfer(bus, &msg, 1U);
}

/**
 * @brief  启动软件 I2C 总线。
 * @param  bus I2C 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   启动时释放 SCL/SDA，确保总线处于空闲状态。
 */
static int sw_bus_start(struct i2c *bus)
{
  if ((bus == NULL) || (bus->config == NULL) || (bus->config->speed_hz == 0U)) return PLATFORM_ERR_INVALID_CONFIGURATION;
  pin_mode(bus->config->scl_pin, PIN_MODE_INPUT_PULLUP); pin_mode(bus->config->sda_pin, PIN_MODE_INPUT_PULLUP);
  return PLATFORM_OK;
}
/**
 * @brief  停止软件 I2C 总线。
 * @param  bus I2C 基类对象，当前未使用。
 * @return PLATFORM_OK 表示成功。
 */
static int sw_bus_stop(struct i2c *bus) { (void)bus; return PLATFORM_OK; }

/* 软件 I2C 只提供生命周期和阻塞能力，不伪装成异步实现。 */
static const struct i2c_ops g_sw_ops = {
  .start = sw_bus_start,
  .stop = sw_bus_stop,
};
static const struct i2c_blocking_ops g_sw_blocking_ops = {.transfer = sw_transfer, .mem_read = sw_mem_read, .mem_write = sw_mem_write};

/**
 * @brief  注册 STM32F4 软件 I2C 后端。
 * @param  bus_id platform I2C 控制器编号。
 * @param  config 板级 I2C 总线配置。
 * @return PLATFORM_OK 表示注册成功；其他 PLATFORM_* 表示失败。
 */
int stm32f4_i2c_sw_register(int32_t bus_id, const struct i2c_bus_config *config)
{
  struct stm32f4_i2c_sw_bus *bus;
  if ((bus_id < STM32F4_I2C1) || (bus_id > STM32F4_I2C3) || (config == NULL)) return PLATFORM_ERR_INVALID_ARGUMENT;
  bus = &g_sw_i2c_table[bus_id]; bus->half_period_us = STM32F4_SW_I2C_DEFAULT_HALF_PERIOD_US;
  return i2c_bus_register(&bus->base, config, &g_sw_ops, &g_sw_blocking_ops, NULL);
}
