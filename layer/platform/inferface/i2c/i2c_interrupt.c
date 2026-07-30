/* SPDX-License-Identifier: MIT */
/**
 * @file    i2c_interrupt.c
 * @brief   I2C 异步事务能力包装。
 * @author  weixin
 */
#include "platform/inferface/i2c/i2c_interrupt.h"

#include "result.h"

#include <stddef.h>

/**
 * @brief  检查 I2C 异步事务能力是否可用。
 * @param  bus I2C 总线对象。
 * @return true 表示对象已启动，false 表示不可用。
 */
static bool i2c_interrupt_is_ready(const struct i2c *bus)
{
  return (bus != NULL) && bus->initialized;
}

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
                    void *context)
{
  if ((bus == NULL) || (msgs == NULL) || (num == 0U) ||
      (callback == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!i2c_interrupt_is_ready(bus)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  if ((bus->interrupt_ops == NULL) ||
      (bus->interrupt_ops->transfer == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }
  return bus->interrupt_ops->transfer(bus, msgs, num, callback, context);
}
