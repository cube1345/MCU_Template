/**
 * @file    stm32f4_i2c.h
 * @brief   STM32F4 I2C 后端注册接口。
 * @author  weixin
 */
#ifndef STM32F4_I2C_H
#define STM32F4_I2C_H

#include "platform/inferface/i2c/i2c.h"

/**
 * @brief  注册 STM32F4 硬件 I2C 后端。
 * @param  bus_id I2C 总线编号。
 * @param  config 板级传入的 I2C 总线配置。
 * @return PLATFORM_OK 表示注册成功；其他值表示参数无效或注册失败。
 * @note   该后端使用 STM32F4 I2C 外设和 HAL，启动时会接管时钟、
 *         GPIO 复用和 I2C 外设资源。
 */
int stm32f4_i2c_register(int32_t bus_id, const struct i2c_bus_config *config);

/**
 * @brief  注册 STM32F4 软件 I2C 后端。
 * @param  bus_id I2C 总线编号。
 * @param  config 板级传入的 I2C 总线配置。
 * @return PLATFORM_OK 表示注册成功；其他值表示参数无效或注册失败。
 * @note   该后端使用 GPIO 位操作模拟 I2C，当前提供阻塞传输能力。
 */
int stm32f4_i2c_sw_register(int32_t bus_id, const struct i2c_bus_config *config);

#endif
