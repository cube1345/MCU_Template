/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c_interrupt.h
 * @brief   I2C 异步事务固定能力接口。
 * @author  weixin
 *
 * 回调通常运行于 I2C IRQ 上下文，不能阻塞、调用 HAL_Delay() 或启动同一总线的
 * 新事务。缓冲区必须在回调触发前保持有效。
 */
#ifndef I2C_INTERRUPT_H
#define I2C_INTERRUPT_H

#include "platform/inferface/i2c/i2c.h"

typedef void (*i2c_interrupt_callback_t)(struct i2c *self,
                                         int result,
                                         void *context);

struct i2c_interrupt_ops {
  int (*transfer)(struct i2c *self,
                  struct i2c_msg *msgs,
                  uint32_t num,
                  i2c_interrupt_callback_t callback,
                  void *context);
};

/**
 * @brief  启动一次 I2C 异步消息传输。
 * @param  bus 已启动的 I2C 总线对象。
 * @param  msgs I2C 消息数组，完成前必须保持有效。
 * @param  num 消息数量。
 * @param  callback 完成或错误回调。
 * @param  context 调用者上下文指针，平台层不解释。
 * @return PLATFORM_OK 表示启动成功；其他 PLATFORM_* 表示失败。
 */
int i2c_transfer_it(struct i2c *bus,
                    struct i2c_msg *msgs,
                    uint32_t num,
                    i2c_interrupt_callback_t callback,
                    void *context);

#endif /* I2C_INTERRUPT_H */
