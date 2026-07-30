/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c.h
 * @brief   Platform I2C 基础对象与生命周期接口。
 * @author  weixin
 *
 * I2C 是中等复杂度设备：本文件只定义基础对象、总线注册与生命周期；阻塞和
 * 异步事务分别由 i2c_blocking.h、i2c_interrupt.h 提供固定能力接口。
 */
#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

#define PLATFORM_I2C_WR UINT16_C(0x0000)
#define PLATFORM_I2C_RD UINT16_C(0x0001)
#define PLATFORM_I2C_NO_START UINT16_C(0x0002)

#define PLATFORM_I2C_BUS_TABLE_SIZE UINT8_C(4)
#define PLATFORM_I2C_MEM_ADDR_SIZE_8BIT UINT8_C(1)
#define PLATFORM_I2C_MEM_ADDR_SIZE_16BIT UINT8_C(2)

struct i2c;
struct i2c_blocking_ops;
struct i2c_interrupt_ops;

/* 一条 I2C 消息；addr 为未左移的 7 位从机地址。 */
struct i2c_msg {
  uint16_t addr;
  uint16_t flags;
  uint16_t len;
  uint8_t *buf;
};

/* 板级传入的 SCL/SDA 连线、总线速率和阻塞超时配置。 */
struct i2c_bus_config {
  int32_t scl_pin;
  int32_t sda_pin;
  uint32_t speed_hz;
  uint32_t timeout_ms;
};

struct i2c_ops {
  int (*start)(struct i2c *self);
  int (*stop)(struct i2c *self);
};

struct i2c {
  int32_t id;
  const char *name;
  const struct i2c_bus_config *config;
  const struct i2c_ops *ops;
  const struct i2c_blocking_ops *blocking_ops;
  const struct i2c_interrupt_ops *interrupt_ops;
  bool initialized;
};

/**
 * @brief  注册并启动 I2C 总线对象。
 * @param  bus 调用者提供的 I2C 总线对象。
 * @param  config 板级 I2C 引脚、速率和超时配置。
 * @param  ops 基础生命周期操作表。
 * @param  blocking_ops 阻塞事务能力表，不支持时可传 NULL。
 * @param  interrupt_ops 异步事务能力表，不支持时可传 NULL。
 * @return PLATFORM_OK 表示注册成功；其他 PLATFORM_* 表示失败。
 */
int i2c_bus_register(struct i2c *bus,
                     const struct i2c_bus_config *config,
                     const struct i2c_ops *ops,
                     const struct i2c_blocking_ops *blocking_ops,
                     const struct i2c_interrupt_ops *interrupt_ops);

/**
 * @brief  获取默认 I2C 总线。
 * @return 默认 I2C 总线对象；没有注册总线时返回 NULL。
 */
struct i2c *i2c_bus_get(void);

/**
 * @brief  通过总线编号获取 I2C 总线对象。
 * @param  id I2C 总线编号。
 * @return 对应 I2C 总线对象；编号无效或未注册时返回 NULL。
 */
struct i2c *i2c_bus_get_by_id(int32_t id);

/**
 * @brief  启动 I2C 总线对象。
 * @param  bus 已注册 ops 和配置的 I2C 总线对象。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
int i2c_bus_start(struct i2c *bus);

/**
 * @brief  停止 I2C 总线对象并释放硬件资源。
 * @param  bus 已启动的 I2C 总线对象。
 * @return PLATFORM_OK 表示释放成功；其他 PLATFORM_* 表示失败。
 */
int i2c_bus_stop(struct i2c *bus);

#endif /* I2C_H */
