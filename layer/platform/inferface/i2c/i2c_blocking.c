/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c_blocking.c
 * @brief   I2C 阻塞事务能力包装。
 * @author  weixin
 */
#include "platform/inferface/i2c/i2c_blocking.h"

#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 I2C 阻塞事务能力是否可用。
 * @param  bus I2C 总线对象。
 * @return true 表示对象已启动，false 表示不可用。
 */
static bool i2c_blocking_is_ready(const struct i2c *bus)
{
  return (bus != NULL) && bus->initialized;
}

/**
 * @brief  发起阻塞 I2C 消息传输。
 * @param  bus 已启动的 I2C 总线对象。
 * @param  msgs I2C 消息数组。
 * @param  num 消息数量。
 * @return PLATFORM_OK 表示传输成功；其他 PLATFORM_* 表示失败。
 */
int i2c_transfer(struct i2c *bus, struct i2c_msg *msgs, uint32_t num)
{
  if ((bus == NULL) || (msgs == NULL) || (num == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!i2c_blocking_is_ready(bus)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((bus->blocking_ops == NULL) ||
      (bus->blocking_ops->transfer == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return bus->blocking_ops->transfer(bus, msgs, num);
}

/**
 * @brief  阻塞读取 I2C 设备寄存器或存储地址。
 * @param  bus 已启动的 I2C 总线对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 接收缓冲区。
 * @param  size 读取字节数。
 * @return PLATFORM_OK 表示读取成功；其他 PLATFORM_* 表示失败。
 */
int i2c_mem_read(struct i2c *bus,
                 const struct i2c_mem_address *address,
                 uint8_t *data, uint16_t size)
{
  if ((bus == NULL) || (address == NULL) || (data == NULL) ||
      (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!i2c_blocking_is_ready(bus)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((bus->blocking_ops == NULL) ||
      (bus->blocking_ops->mem_read == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return bus->blocking_ops->mem_read(bus, address, data, size);
}

/**
 * @brief  阻塞写入 I2C 设备寄存器或存储地址。
 * @param  bus 已启动的 I2C 总线对象。
 * @param  address I2C 设备地址和内部地址。
 * @param  data 待写入数据缓冲区。
 * @param  size 写入字节数。
 * @return PLATFORM_OK 表示写入成功；其他 PLATFORM_* 表示失败。
 */
int i2c_mem_write(struct i2c *bus,
                  const struct i2c_mem_address *address,
                  const uint8_t *data, uint16_t size)
{
  if ((bus == NULL) || (address == NULL) || (data == NULL) ||
      (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!i2c_blocking_is_ready(bus)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((bus->blocking_ops == NULL) ||
      (bus->blocking_ops->mem_write == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return bus->blocking_ops->mem_write(bus, address, data, size);
}
