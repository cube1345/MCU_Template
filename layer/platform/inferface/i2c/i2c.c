/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c.c
 * @brief   I2C 基础对象注册与生命周期管理。
 * @author  weixin
 */
#include "platform/inferface/i2c/i2c.h"

#include "result.h"

#include <stddef.h>

static struct i2c *g_bus_table[PLATFORM_I2C_BUS_TABLE_SIZE];
static struct i2c *g_default_bus;

/**
 * @brief  检查 I2C 总线对象是否已经启动。
 * @param  bus I2C 总线对象。
 * @return true 表示对象可用，false 表示对象无效或未启动。
 */
static bool i2c_bus_is_ready(const struct i2c *bus)
{
  return (bus != NULL) && bus->initialized && (bus->ops != NULL);
}

/**
 * @brief  检查 I2C 总线编号是否在注册表范围内。
 * @param  id I2C 总线编号。
 * @return true 表示编号有效，false 表示编号越界。
 */
static bool i2c_bus_id_is_valid(int32_t id)
{
  return (id >= 0) && (id < (int32_t)PLATFORM_I2C_BUS_TABLE_SIZE);
}

/**
 * @brief  启动 I2C 总线对象。
 * @param  bus 已注册 ops 和配置的 I2C 总线对象。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 * @note   重复启动保持幂等。
 */
int i2c_bus_start(struct i2c *bus)
{
  int result;

  if ((bus == NULL) || (bus->ops == NULL) ||
      (bus->ops->start == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (bus->initialized) {
    return PLATFORM_OK;
  }
  result = bus->ops->start(bus);
  if (result == PLATFORM_OK) {
    bus->initialized = true;
  }
  return result;
}

/**
 * @brief  停止 I2C 总线对象并释放硬件资源。
 * @param  bus 已启动的 I2C 总线对象。
 * @return PLATFORM_OK 表示释放成功；其他 PLATFORM_* 表示失败。
 */
int i2c_bus_stop(struct i2c *bus)
{
  if (!i2c_bus_is_ready(bus) || (bus->ops->stop == NULL)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if (bus->ops->stop(bus) != PLATFORM_OK) {
    return PLATFORM_ERR_IO;
  }
  bus->initialized = false;
  return PLATFORM_OK;
}

/**
 * @brief  注册并启动 I2C 总线对象。
 * @param  bus 调用者提供的 I2C 总线对象。
 * @param  config 板级 I2C 引脚、速率和超时配置。
 * @param  ops 基础生命周期操作表。
 * @param  blocking_ops 阻塞事务能力表，不支持时可传 NULL。
 * @param  interrupt_ops 异步事务能力表，不支持时可传 NULL。
 * @return PLATFORM_OK 表示注册成功；其他 PLATFORM_* 表示失败。
 * @note   启动失败会撤销能力表绑定，避免留下半启动对象。
 */
int i2c_bus_register(struct i2c *bus,
                     const struct i2c_bus_config *config,
                     const struct i2c_ops *ops,
                     const struct i2c_blocking_ops *blocking_ops,
                     const struct i2c_interrupt_ops *interrupt_ops)
{
  if ((bus == NULL) || (config == NULL) || (ops == NULL) ||
      (ops->start == NULL) || (ops->stop == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!i2c_bus_id_is_valid(bus->id)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((g_bus_table[bus->id] != NULL) &&
      (g_bus_table[bus->id] != bus)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  bus->config = config;
  bus->ops = ops;
  bus->blocking_ops = blocking_ops;
  bus->interrupt_ops = interrupt_ops;
  bus->initialized = false;
  if (i2c_bus_start(bus) != PLATFORM_OK) {
    bus->config = NULL;
    bus->ops = NULL;
    bus->blocking_ops = NULL;
    bus->interrupt_ops = NULL;
    return PLATFORM_ERR_IO;
  }

  g_bus_table[bus->id] = bus;
  if (g_default_bus == NULL) {
    g_default_bus = bus;
  }
  return PLATFORM_OK;
}

/**
 * @brief  获取默认 I2C 总线。
 * @return 默认 I2C 总线对象；没有注册总线时返回 NULL。
 */
struct i2c *i2c_bus_get(void)
{
  return g_default_bus;
}

/**
 * @brief  通过总线编号获取 I2C 总线对象。
 * @param  id I2C 总线编号。
 * @return 对应 I2C 总线对象；编号无效或未注册时返回 NULL。
 */
struct i2c *i2c_bus_get_by_id(int32_t id)
{
  if (!i2c_bus_id_is_valid(id)) {
    return NULL;
  }
  return g_bus_table[id];
}
