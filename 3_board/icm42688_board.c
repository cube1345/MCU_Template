/**
 * @file    icm42688_board.c
 * @brief   ICM42688 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 用公共 device_iic_t 绑定默认 IIC 总线和器件地址（0x69）
 * - 通过 icm42688_cfg_t.iic 将 I2C 传输契约注入 Device 层，消除 Device→Board 反向依赖
 * - 实例化 g_icm42688 设备对象，绑定默认传感器配置
 * - 提供 ICM42688_Board_Init() 和 ICM42688_Board_GetDevice()
 *
 * 原先独立的 icm42688_int.c 已合并到此文件。
 */

#include "icm42688_board.h"
#include "iic_board.h"

#define ICM42688_INT_DEVICE_ADDR    (0x69U)

static icm42688_t g_icm42688;

static const icm42688_cfg_t g_icm42688_cfg = {
    .init_config = {
        .gyro_fs = ICM42688_GYRO_FS_2000DPS,
        .gyro_odr = ICM42688_GYRO_ODR_1KHZ,
        .accel_fs = ICM42688_ACCEL_FS_4G,
        .accel_odr = ICM42688_ACCEL_ODR_1KHZ,
    },
    .iic = {
        .ops = &g_iic_board_default_ops,
        .context = (void *)0,
        .address = ICM42688_INT_DEVICE_ADDR,
    },
};

icm42688_t *ICM42688_Board_GetDevice(void)
{
    return &g_icm42688;
}

uint8_t ICM42688_Board_GetDeviceAddr(void)
{
    return ICM42688_INT_DEVICE_ADDR;
}

void ICM42688_Board_Init(void)
{
    ICM42688_InitDevice(&g_icm42688, &g_icm42688_cfg);
}
