/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c_blocking.h
 * @brief   I2C 阻塞事务固定能力接口。
 * @author  weixin
 *
 * 这不是应用层可见的具体 I2C 子类，而是中等复杂度 I2C 的固定能力表。
 * 应用层仍只持有 struct i2c *，使用本文件的包装函数发起阻塞事务。
 */
#ifndef I2C_BLOCKING_H
#define I2C_BLOCKING_H

#include "platform/inferface/i2c/i2c.h"

struct i2c_mem_address {
  uint16_t device_addr;
  uint16_t memory_addr;
  uint8_t memory_addr_size;
};

struct i2c_blocking_ops {
  int (*transfer)(struct i2c *self, struct i2c_msg *msgs, uint32_t num);
  int (*mem_read)(struct i2c *self,
                  const struct i2c_mem_address *address,
                  uint8_t *data, uint16_t size);
  int (*mem_write)(struct i2c *self,
                   const struct i2c_mem_address *address,
                   const uint8_t *data, uint16_t size);
};

/**
 * @brief  发起阻塞 I2C 消息传输。
 * @param  bus 已启动的 I2C 总线对象。
 * @param  msgs I2C 消息数组。
 * @param  num 消息数量。
 * @return PLATFORM_OK 表示传输成功；其他 PLATFORM_* 表示失败。
 */
int i2c_transfer(struct i2c *bus, struct i2c_msg *msgs, uint32_t num);

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
                 uint8_t *data, uint16_t size);

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
                  const uint8_t *data, uint16_t size);

#endif /* I2C_BLOCKING_H */
