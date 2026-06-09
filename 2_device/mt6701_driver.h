/**
 * @file    mt6701_driver.h
 * @brief   MT6701 14-bit 磁编码器 GPIO SSI 驱动接口 (双编码器)
 * @author  cube
 * @date    2026-05-02
 *
 * 硬件拓扑: 两片 MT6701 共享 SCLK 与 DO, 各自独立 CSN。
 * SSI 协议: CPOL=0, CPHA=1 (SPI Mode 1), MSB first, 24-bit 帧。
 *
 * 分层说明:
 * - Device 层 (本文件): SSI 协议逻辑、CRC 校验、角度换算、速度计算
 * - Board 层 (mt6701_board.c): 定义具体引脚, 通过 mt6701_bus_cfg_t 传入
 * - HAL 层 (hal_gpio.h): GPIO 读写通过 HAL_GPIO_* 接口
 *
 * Device 层不包含任何具体引脚号, 引脚信息由 Board 层通过 MT6701_Init() 注入。
 */

#ifndef __MT6701_DRIVER_H
#define __MT6701_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "hal_gpio.h"
#include "device_base.h"
#include "device_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MT6701 原始角度分辨率 (14-bit) */
#define MT6701_ANGLE_MAX       ((1 << 14) - 1)  /* 16383 */
#define MT6701_ANGLE_BITS      14U
#define MT6701_SSI_FRAME_BITS  24U

/**
 * @brief 编码器轴编号
 */
typedef enum {
    MT6701_AXIS_M1 = 0U,
    MT6701_AXIS_M2 = 1U,
    MT6701_AXIS_COUNT = 2U
} MT6701_Axis_t;

/**
 * @brief 单路 GPIO 引脚描述 (端口 + 引脚号)
 *
 * 由 Board 层根据实际板级接线填充, 通过 mt6701_bus_cfg_t 注入 Device 层。
 */
typedef platform_gpio_t mt6701_gpio_t;

/**
 * @brief MT6701 SSI 总线引脚配置
 *
 * 两片 MT6701 共享 SCLK 和 DO, CSN 按轴独立。
 * Board 层在 MT6701_Board_Init() 中填充此结构体并传给 MT6701_Init()。
 */
typedef struct {
    mt6701_gpio_t sclk;                      /**< 共享 SCLK (时钟输出) */
    mt6701_gpio_t dio;                       /**< 共享 DO  (数据输入) */
    mt6701_gpio_t csn[MT6701_AXIS_COUNT];    /**< 各轴独立 CSN (片选, 低有效) */
} mt6701_bus_cfg_t;

/* OOP 封装 */
typedef struct mt6701_s mt6701_t;
typedef device_magnetic_encoder_ops_t mt6701_base_ops_t;

struct mt6701_s {
    device_base_t base;
    const mt6701_base_ops_t *ops;
    mt6701_bus_cfg_t bus_cfg;
};

void MT6701_InitDevice(mt6701_t *me, const mt6701_bus_cfg_t *bus_cfg);

/**
 * @brief 初始化 MT6701 内部状态并绑定 SSI 总线引脚
 * @param bus_cfg SSI 总线引脚配置, 由 Board 层提供
 *
 * 调用时机: Board 层 MT6701_Board_Init() 先通过 HAL_GPIO_InitOutput/Input
 * 初始化 GPIO IOMUX, 再调用本函数传入引脚配置并初始化内部状态数组。
 */
void MT6701_Init(const mt6701_bus_cfg_t *bus_cfg);

/**
 * @brief 读取指定轴一次原始 14-bit 角度 (阻塞, 约 3µs)
 * @param axis 轴编号
 * @return 0~16383 有效角度, -1 表示 CRC 校验失败
 */
int32_t MT6701_ReadAngleRaw(MT6701_Axis_t axis);

/**
 * @brief 读取指定轴角度 (弧度, 0 ~ 2π)
 */
float MT6701_ReadAngleRad(MT6701_Axis_t axis);

/**
 * @brief 读取指定轴角度 (度, 0 ~ 360)
 */
float MT6701_ReadAngleDeg(MT6701_Axis_t axis);

/**
 * @brief 获取指定轴最近一次的磁场状态
 * @return 4-bit 状态 (bit3=弱磁, bit2=强磁, bit1=push, bit0=speed sat)
 */
uint8_t MT6701_GetFieldStatus(MT6701_Axis_t axis);

/**
 * @brief 获取指定轴最近一次角度原始值 (不触发新读取)
 * @return 上次读取的角度 raw, 未初始化时返回 -1
 */
int32_t MT6701_GetLastAngleRaw(MT6701_Axis_t axis);

/**
 * @brief 获取指定轴最近一次读取的 CRC 是否通过
 */
bool MT6701_IsLastCRCValid(MT6701_Axis_t axis);

/**
 * @brief 1ms 速度计算服务 (由 STEP_CTRL_TIMER ISR 调用)
 *
 * 对两轴依次读取并计算电机速度。
 */
void MT6701_Service1ms(void);

/**
 * @brief 获取指定轴当前电机速度
 * @return 速度, 单位 rad/s
 */
float MT6701_GetSpeed(MT6701_Axis_t axis);

#ifdef __cplusplus
}
#endif

#endif /* __MT6701_DRIVER_H */
