/* SPDX-License-Identifier: MIT */
/**
 * @file    stm32f4_i2c.c
 * @brief   STM32F4 硬件 I2C 后端。
 * @author  weixin
 *
 * 本文件只负责把 Platform I2C 能力映射到 STM32 HAL。I2C 的公共对象、
 * 阻塞能力和中断能力由 platform/i2c.h 定义，应用层不会看到 HAL 句柄。
 */
#include "stm32f4.h"
#include "stm32f4_i2c.h"
#include "container_of.h"
#include "platform/inferface/i2c/i2c.h"
#include "platform/inferface/i2c/i2c_blocking.h"
#include "platform/inferface/i2c/i2c_interrupt.h"
#include "result.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>

#define STM32F4_I2C_SPEED_FAST_HZ UINT32_C(400000)
#define STM32F4_I2C_ADDR_7BIT_MAX UINT16_C(0x007F)
/* I2C 从设备最多可用 9 个 SCL 脉冲释放未完成传输中的 SDA。 */
#define STM32F4_I2C_RECOVERY_CLOCK_COUNT UINT32_C(9)
/* 总线恢复过程中每个电平阶段的保持时间，单位为毫秒。 */
#define STM32F4_I2C_RECOVERY_DELAY_MS UINT32_C(1)

struct stm32f4_i2c_bus {
  struct i2c base;
  I2C_HandleTypeDef handle;
  bool pending;
  i2c_interrupt_callback_t callback;
  void *context;
};

static struct stm32f4_i2c_bus g_i2c_bus_table[] = {
  {.base = {.id = STM32F4_I2C1, .name = "i2c1"}},
  {.base = {.id = STM32F4_I2C2, .name = "i2c2"}},
  {.base = {.id = STM32F4_I2C3, .name = "i2c3"}},
};

/**
 * @brief  判断当前是否处于中断上下文。
 * @return 1 表示当前在 ISR 中，0 表示在线程上下文。
 */
static int stm32f4_i2c_is_in_isr(void)
{
  return (__get_IPSR() != 0U) ? 1 : 0;
}

/**
 * @brief  将 platform 引脚编号转换为 HAL GPIO 端口和位掩码。
 * @param  pin platform 引脚编号。
 * @param  port 用于接收 HAL GPIO 端口指针。
 * @param  mask 用于接收 HAL GPIO 位掩码。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_pin_decode(int32_t pin, GPIO_TypeDef **port,
                                  uint16_t *mask)
{
  int32_t port_id;
  int32_t pin_id;

  if ((pin < 0) || (port == NULL) || (mask == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  port_id = pin / STM32F4_PIN_PORT_WIDTH;
  pin_id = pin % STM32F4_PIN_PORT_WIDTH;
  if ((port_id < STM32F4_PORT_A) || (port_id > STM32F4_PORT_E) ||
      (pin_id < 0) || (pin_id >= STM32F4_PIN_PORT_WIDTH)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (port_id == STM32F4_PORT_A) {
    *port = GPIOA; __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port_id == STM32F4_PORT_B) {
    *port = GPIOB; __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port_id == STM32F4_PORT_C) {
    *port = GPIOC; __HAL_RCC_GPIOC_CLK_ENABLE();
  } else if (port_id == STM32F4_PORT_D) {
    *port = GPIOD; __HAL_RCC_GPIOD_CLK_ENABLE();
  } else {
    *port = GPIOE; __HAL_RCC_GPIOE_CLK_ENABLE();
  }
  *mask = (uint16_t)(UINT16_C(1) << (uint32_t)pin_id);
  return PLATFORM_OK;
}

/**
 * @brief  根据 platform I2C 编号获取 HAL I2C 实例。
 * @param  id platform I2C 控制器编号。
 * @return HAL I2C 实例指针；编号无效时返回 NULL。
 * @note   成功时会同时打开对应 I2C 外设时钟。
 */
static I2C_TypeDef *stm32f4_i2c_instance(int32_t id)
{
  if (id == STM32F4_I2C1) { __HAL_RCC_I2C1_CLK_ENABLE(); return I2C1; }
  if (id == STM32F4_I2C2) { __HAL_RCC_I2C2_CLK_ENABLE(); return I2C2; }
  if (id == STM32F4_I2C3) { __HAL_RCC_I2C3_CLK_ENABLE(); return I2C3; }
  return NULL;
}

/**
 * @brief  校验 I2C 控制器和 SCL/SDA 引脚复用组合是否合法。
 * @param  id platform I2C 控制器编号。
 * @param  config 板级 I2C 总线配置。
 * @param  alternate 用于接收 HAL GPIO 复用功能编号。
 * @return PLATFORM_OK 表示组合合法；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_af_is_valid(int32_t id,
                                   const struct i2c_bus_config *config,
                                   uint32_t *alternate)
{
  if ((config == NULL) || (alternate == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((id == STM32F4_I2C1) &&
      (((config->scl_pin == STM32F4_PIN_PB6) &&
        (config->sda_pin == STM32F4_PIN_PB7)) ||
       ((config->scl_pin == STM32F4_PIN_PB8) &&
        (config->sda_pin == STM32F4_PIN_PB9)))) {
    *alternate = GPIO_AF4_I2C1; return PLATFORM_OK;
  }
  if ((id == STM32F4_I2C2) && (config->scl_pin == STM32F4_PIN_PB10) &&
      (config->sda_pin == STM32F4_PIN_PB11)) {
    *alternate = GPIO_AF4_I2C2; return PLATFORM_OK;
  }
  if ((id == STM32F4_I2C3) && (config->scl_pin == STM32F4_PIN_PA8) &&
      (config->sda_pin == STM32F4_PIN_PC9)) {
    *alternate = GPIO_AF4_I2C3; return PLATFORM_OK;
  }
  return PLATFORM_ERR_INVALID_CONFIGURATION;
}

/**
 * @brief  初始化 I2C SCL/SDA 复用 GPIO。
 * @param  bus I2C 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_gpio_init(const struct i2c *bus)
{
  GPIO_InitTypeDef pin = {0};
  GPIO_TypeDef *scl_port;
  GPIO_TypeDef *sda_port;
  uint16_t scl_mask;
  uint16_t sda_mask;
  uint32_t alternate;

  if ((bus == NULL) || (bus->config == NULL) ||
      (stm32f4_i2c_af_is_valid(bus->id, bus->config, &alternate) != PLATFORM_OK) ||
      (stm32f4_i2c_pin_decode(bus->config->scl_pin, &scl_port, &scl_mask) != PLATFORM_OK) ||
      (stm32f4_i2c_pin_decode(bus->config->sda_pin, &sda_port, &sda_mask) != PLATFORM_OK)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  pin.Mode = GPIO_MODE_AF_OD;
  pin.Pull = GPIO_PULLUP;
  pin.Speed = GPIO_SPEED_FREQ_HIGH;
  pin.Alternate = alternate;
  pin.Pin = scl_mask; HAL_GPIO_Init(scl_port, &pin);
  pin.Pin = sda_mask; HAL_GPIO_Init(sda_port, &pin);
  return PLATFORM_OK;
}

/**
 * @brief  在启动 I2C 外设前恢复可能被从设备占用的总线。
 * @param  bus：包含 SCL、SDA 引脚配置的 I2C 基础对象。
 * @retval PLATFORM_OK：SCL 和 SDA 均已恢复为空闲高电平。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：引脚配置无法解析。
 * @retval PLATFORM_ERR_BUSY：恢复后仍有引脚被外部设备拉低。
 * @note   仅在总线启动阶段调用；恢复期间引脚临时配置为开漏输出。
 */
static int stm32f4_i2c_bus_recover(const struct i2c *bus)
{
  GPIO_InitTypeDef pin = {0};
  GPIO_TypeDef *scl_port;
  GPIO_TypeDef *sda_port;
  uint16_t scl_mask;
  uint16_t sda_mask;
  uint32_t clock_count;

  if ((bus == NULL) || (bus->config == NULL) ||
      (stm32f4_i2c_pin_decode(bus->config->scl_pin,
                              &scl_port, &scl_mask) != PLATFORM_OK) ||
      (stm32f4_i2c_pin_decode(bus->config->sda_pin,
                              &sda_port, &sda_mask) != PLATFORM_OK)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  /*
   * 先以开漏输出释放两根线。开漏高电平不会主动驱动总线，
   * 因而可以安全检查是否仍有从设备持续拉低 SCL 或 SDA。
   */
  HAL_GPIO_WritePin(scl_port, scl_mask, GPIO_PIN_SET);
  HAL_GPIO_WritePin(sda_port, sda_mask, GPIO_PIN_SET);
  pin.Mode = GPIO_MODE_OUTPUT_OD;
  pin.Pull = GPIO_PULLUP;
  pin.Speed = GPIO_SPEED_FREQ_HIGH;
  pin.Pin = scl_mask;
  HAL_GPIO_Init(scl_port, &pin);
  pin.Pin = sda_mask;
  HAL_GPIO_Init(sda_port, &pin);
  HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);

  if (HAL_GPIO_ReadPin(scl_port, scl_mask) == GPIO_PIN_RESET) {
    return PLATFORM_ERR_BUSY;
  }

  /*
   * SDA 被拉低通常表示从设备停留在未完成的发送状态。主机补发
   * 最多 9 个时钟，使从设备移出当前字节并释放 SDA。
   */
  for (clock_count = 0U;
       (clock_count < STM32F4_I2C_RECOVERY_CLOCK_COUNT) &&
       (HAL_GPIO_ReadPin(sda_port, sda_mask) == GPIO_PIN_RESET);
       clock_count++) {
    HAL_GPIO_WritePin(scl_port, scl_mask, GPIO_PIN_RESET);
    HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);
    HAL_GPIO_WritePin(scl_port, scl_mask, GPIO_PIN_SET);
    HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);
    if (HAL_GPIO_ReadPin(scl_port, scl_mask) == GPIO_PIN_RESET) {
      return PLATFORM_ERR_BUSY;
    }
  }

  /* 主动生成一次 STOP 条件，结束从设备可能保留的旧事务。 */
  HAL_GPIO_WritePin(sda_port, sda_mask, GPIO_PIN_RESET);
  HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);
  HAL_GPIO_WritePin(scl_port, scl_mask, GPIO_PIN_SET);
  HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);
  HAL_GPIO_WritePin(sda_port, sda_mask, GPIO_PIN_SET);
  HAL_Delay(STM32F4_I2C_RECOVERY_DELAY_MS);

  if ((HAL_GPIO_ReadPin(scl_port, scl_mask) == GPIO_PIN_RESET) ||
      (HAL_GPIO_ReadPin(sda_port, sda_mask) == GPIO_PIN_RESET)) {
    return PLATFORM_ERR_BUSY;
  }
  return PLATFORM_OK;
}

/**
 * @brief  复位指定 I2C 外设的寄存器状态。
 * @param  id：STM32F4_I2C1 至 STM32F4_I2C3 控制器编号。
 * @retval None。
 */
static void stm32f4_i2c_peripheral_reset(int32_t id)
{
  if (id == STM32F4_I2C1) {
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
  } else if (id == STM32F4_I2C2) {
    __HAL_RCC_I2C2_FORCE_RESET();
    __HAL_RCC_I2C2_RELEASE_RESET();
  } else if (id == STM32F4_I2C3) {
    __HAL_RCC_I2C3_FORCE_RESET();
    __HAL_RCC_I2C3_RELEASE_RESET();
  }
}

/**
 * @brief  启动 STM32F4 硬件 I2C 总线。
 * @param  bus I2C 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_start(struct i2c *bus)
{
  struct stm32f4_i2c_bus *self;
  I2C_TypeDef *instance;

  if ((bus == NULL) || (bus->config == NULL) ||
      (bus->config->speed_hz == 0U) ||
      (bus->config->speed_hz > STM32F4_I2C_SPEED_FAST_HZ) ||
      (bus->config->timeout_ms == 0U)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  if (stm32f4_i2c_bus_recover(bus) != PLATFORM_OK) {
    return PLATFORM_ERR_BUSY;
  }
  if (stm32f4_i2c_gpio_init(bus) != PLATFORM_OK) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  instance = stm32f4_i2c_instance(bus->id);
  if (instance == NULL) return PLATFORM_ERR_INVALID_ARGUMENT;
  /*
   * 总线已经恢复为空闲状态后再清除外设内部状态，避免上一次复位或
   * 异常事务遗留的 BUSY 标志影响第一次 HAL 传输。
   */
  stm32f4_i2c_peripheral_reset(bus->id);
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  self->handle.Instance = instance;
  self->handle.Init.ClockSpeed = bus->config->speed_hz;
  self->handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
  self->handle.Init.OwnAddress1 = 0U;
  self->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  self->handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  self->handle.Init.OwnAddress2 = 0U;
  self->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  self->handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  self->pending = false;
  return (HAL_I2C_Init(&self->handle) == HAL_OK) ? PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  停止 STM32F4 硬件 I2C 总线。
 * @param  bus I2C 基类对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_stop(struct i2c *bus)
{
  struct stm32f4_i2c_bus *self;
  if (bus == NULL) return PLATFORM_ERR_INVALID_ARGUMENT;
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  self->pending = false;
  return (HAL_I2C_DeInit(&self->handle) == HAL_OK) ? PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  校验 I2C 消息参数。
 * @param  msg 待校验的 I2C 消息。
 * @return PLATFORM_OK 表示合法；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_msg_valid(const struct i2c_msg *msg)
{
  if ((msg == NULL) || (msg->addr > STM32F4_I2C_ADDR_7BIT_MAX) ||
      ((msg->len > 0U) && (msg->buf == NULL))) return PLATFORM_ERR_INVALID_ARGUMENT;
  if ((msg->flags & PLATFORM_I2C_NO_START) != 0U) return PLATFORM_ERR_UNSUPPORTED;
  return PLATFORM_OK;
}

/**
 * @brief  将 HAL I2C 状态转换为 platform 错误码。
 * @param  status HAL 返回状态。
 * @return PLATFORM_OK、PLATFORM_ERR_TIMEOUT、PLATFORM_ERR_BUSY 或 IO 错误。
 */
static int stm32f4_i2c_hal_result(HAL_StatusTypeDef status)
{
  if (status == HAL_OK) return PLATFORM_OK;
  if (status == HAL_TIMEOUT) return PLATFORM_ERR_TIMEOUT;
  if (status == HAL_BUSY) return PLATFORM_ERR_BUSY;
  return PLATFORM_ERR_IO;
}

/**
 * @brief  执行阻塞 I2C 主机收发消息。
 * @param  bus I2C 基类对象。
 * @param  msgs I2C 消息数组。
 * @param  num 消息数量。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_transfer(struct i2c *bus, struct i2c_msg *msgs, uint32_t num)
{
  struct stm32f4_i2c_bus *self;
  uint32_t i;
  HAL_StatusTypeDef status;
  if ((bus == NULL) || (msgs == NULL) || (num == 0U)) return PLATFORM_ERR_INVALID_ARGUMENT;
  if (stm32f4_i2c_is_in_isr()) return PLATFORM_ERR_ISR_CONTEXT;
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  for (i = 0U; i < num; ++i) {
    if (stm32f4_i2c_msg_valid(&msgs[i]) != PLATFORM_OK) return PLATFORM_ERR_INVALID_ARGUMENT;
    if ((msgs[i].flags & PLATFORM_I2C_RD) != 0U) {
      status = HAL_I2C_Master_Receive(&self->handle, (uint16_t)(msgs[i].addr << 1U),
                                      msgs[i].buf, msgs[i].len, bus->config->timeout_ms);
    } else {
      status = HAL_I2C_Master_Transmit(&self->handle, (uint16_t)(msgs[i].addr << 1U),
                                       msgs[i].buf, msgs[i].len, bus->config->timeout_ms);
    }
    if (stm32f4_i2c_hal_result(status) != PLATFORM_OK) return stm32f4_i2c_hal_result(status);
  }
  return PLATFORM_OK;
}

/**
 * @brief  使用 HAL 阻塞读取 I2C 设备寄存器。
 * @param  bus I2C 基类对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 接收缓冲区。
 * @param  size 读取字节数。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_mem_read(struct i2c *bus, const struct i2c_mem_address *address,
                                uint8_t *data, uint16_t size)
{
  struct stm32f4_i2c_bus *self;
  uint16_t mem_size;
  HAL_StatusTypeDef status;
  if ((bus == NULL) || (address == NULL) || (data == NULL) || (size == 0U) ||
      (address->device_addr > STM32F4_I2C_ADDR_7BIT_MAX) ||
      ((address->memory_addr_size != PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) &&
       (address->memory_addr_size != PLATFORM_I2C_MEM_ADDR_SIZE_16BIT)) ||
      ((address->memory_addr_size == PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) &&
       (address->memory_addr > UINT16_C(0xFF)))) return PLATFORM_ERR_INVALID_ARGUMENT;
  if (stm32f4_i2c_is_in_isr()) return PLATFORM_ERR_ISR_CONTEXT;
  mem_size = (address->memory_addr_size == PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) ?
             I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  status = HAL_I2C_Mem_Read(&self->handle, (uint16_t)(address->device_addr << 1U),
                            address->memory_addr, mem_size, data, size,
                            bus->config->timeout_ms);
  return stm32f4_i2c_hal_result(status);
}

/**
 * @brief  使用 HAL 阻塞写入 I2C 设备寄存器。
 * @param  bus I2C 基类对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 待写入数据缓冲区。
 * @param  size 写入字节数。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_mem_write(struct i2c *bus, const struct i2c_mem_address *address,
                                 const uint8_t *data, uint16_t size)
{
  struct stm32f4_i2c_bus *self;
  uint16_t mem_size;
  HAL_StatusTypeDef status;
  if ((bus == NULL) || (address == NULL) || (data == NULL) || (size == 0U) ||
      (address->device_addr > STM32F4_I2C_ADDR_7BIT_MAX) ||
      ((address->memory_addr_size != PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) &&
       (address->memory_addr_size != PLATFORM_I2C_MEM_ADDR_SIZE_16BIT)) ||
      ((address->memory_addr_size == PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) &&
       (address->memory_addr > UINT16_C(0xFF)))) return PLATFORM_ERR_INVALID_ARGUMENT;
  if (stm32f4_i2c_is_in_isr()) return PLATFORM_ERR_ISR_CONTEXT;
  mem_size = (address->memory_addr_size == PLATFORM_I2C_MEM_ADDR_SIZE_8BIT) ?
             I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  status = HAL_I2C_Mem_Write(&self->handle, (uint16_t)(address->device_addr << 1U),
                             address->memory_addr, mem_size, (uint8_t *)data, size,
                             bus->config->timeout_ms);
  return stm32f4_i2c_hal_result(status);
}

/**
 * @brief  启动一次 HAL I2C 中断方式主机收发。
 * @param  bus I2C 基类对象。
 * @param  msgs I2C 消息数组，当前后端只支持单条消息。
 * @param  num 消息数量，必须为 1。
 * @param  callback 完成或错误回调。
 * @param  context 调用者上下文指针。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
static int stm32f4_i2c_transfer_it(struct i2c *bus, struct i2c_msg *msgs, uint32_t num,
                                   i2c_interrupt_callback_t callback, void *context)
{
  struct stm32f4_i2c_bus *self;
  HAL_StatusTypeDef status;
  if ((bus == NULL) || (msgs == NULL) || (num != 1U) || (callback == NULL))
    return PLATFORM_ERR_INVALID_ARGUMENT;
  if (stm32f4_i2c_msg_valid(&msgs[0]) != PLATFORM_OK) return PLATFORM_ERR_INVALID_ARGUMENT;
  self = container_of(bus, struct stm32f4_i2c_bus, base);
  if (self->pending) return PLATFORM_ERR_BUSY;
  self->pending = true; self->callback = callback; self->context = context;
  if ((msgs[0].flags & PLATFORM_I2C_RD) != 0U) {
    status = HAL_I2C_Master_Receive_IT(&self->handle, (uint16_t)(msgs[0].addr << 1U),
                                       msgs[0].buf, msgs[0].len);
  } else {
    status = HAL_I2C_Master_Transmit_IT(&self->handle, (uint16_t)(msgs[0].addr << 1U),
                                        msgs[0].buf, msgs[0].len);
  }
  if (status != HAL_OK) { self->pending = false; return stm32f4_i2c_hal_result(status); }
  return PLATFORM_OK;
}

static const struct i2c_ops g_i2c_ops = {
  .start = stm32f4_i2c_start,
  .stop = stm32f4_i2c_stop,
};
static const struct i2c_blocking_ops g_i2c_blocking_ops = {
  .transfer = stm32f4_i2c_transfer,
  .mem_read = stm32f4_i2c_mem_read,
  .mem_write = stm32f4_i2c_mem_write,
};
static const struct i2c_interrupt_ops g_i2c_interrupt_ops = {
  .transfer = stm32f4_i2c_transfer_it,
};

/**
 * @brief  注册 STM32F4 硬件 I2C 后端。
 * @param  bus_id platform I2C 控制器编号。
 * @param  config 板级 I2C 总线配置。
 * @return PLATFORM_OK 表示注册成功；其他 PLATFORM_* 表示失败。
 */
int stm32f4_i2c_register(int32_t bus_id, const struct i2c_bus_config *config)
{
  if ((bus_id < STM32F4_I2C1) || (bus_id > STM32F4_I2C3)) return PLATFORM_ERR_INVALID_ARGUMENT;
  return i2c_bus_register(&g_i2c_bus_table[bus_id].base, config,
                          &g_i2c_ops, &g_i2c_blocking_ops, &g_i2c_interrupt_ops);
}

/**
 * @brief  完成一次 I2C 中断事务并触发上层回调。
 * @param  handle HAL I2C 句柄。
 * @param  result platform 事务结果。
 * @return 无返回值。
 * @note   该函数运行在 HAL I2C 回调上下文。
 */
static void stm32f4_i2c_complete(I2C_HandleTypeDef *handle, int result)
{
  uint32_t i;
  for (i = 0U; i < (sizeof(g_i2c_bus_table) / sizeof(g_i2c_bus_table[0])); ++i) {
    if (&g_i2c_bus_table[i].handle == handle) {
      if (g_i2c_bus_table[i].pending && g_i2c_bus_table[i].callback != NULL) {
        i2c_interrupt_callback_t callback = g_i2c_bus_table[i].callback;
        void *context = g_i2c_bus_table[i].context;
        g_i2c_bus_table[i].pending = false;
        g_i2c_bus_table[i].callback = NULL;
        g_i2c_bus_table[i].context = NULL;
        callback(&g_i2c_bus_table[i].base, result, context);
      }
      return;
    }
  }
}

/**
 * @brief  HAL I2C 主机发送完成回调。
 * @param  handle HAL I2C 句柄。
 * @return 无返回值。
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *handle)
{ stm32f4_i2c_complete(handle, PLATFORM_OK); }
/**
 * @brief  HAL I2C 主机接收完成回调。
 * @param  handle HAL I2C 句柄。
 * @return 无返回值。
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *handle)
{ stm32f4_i2c_complete(handle, PLATFORM_OK); }
/**
 * @brief  HAL I2C 错误回调。
 * @param  handle HAL I2C 句柄。
 * @return 无返回值。
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle)
{ stm32f4_i2c_complete(handle, PLATFORM_ERR_IO); }
