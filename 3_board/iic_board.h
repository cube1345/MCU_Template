/**
 * @file    iic_board.h
 * @brief   当前板级 IIC 实例与中断绑定。
 * @author  cube
 * @date    2026-08-03
 *
 * Board 层是 SysConfig 生成宏与通用 IIC HAL 之间的唯一连接点。
 */

#ifndef __IIC_BOARD_H
#define __IIC_BOARD_H

#include <ti/driverlib/dl_i2c.h>

#include "device_iic.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 初始化当前板上已配置的 IIC 控制器并开启对应 IRQ。 */
void IIC_Board_Init(void);

/**
 * @brief 获取当前板默认 IIC 控制器。
 * @return 首个已配置的 IIC 实例；未配置时返回空指针。
 */
I2C_Regs *IIC_Board_GetDefault(void);

/**
 * @brief Core IIC transport operations backed by the Board default IIC bus.
 * @note  A device descriptor may set context to a different I2C_Regs instance.
 */
extern const device_iic_ops_t g_iic_board_default_ops;

#ifdef __cplusplus
}
#endif

#endif /* __IIC_BOARD_H */
