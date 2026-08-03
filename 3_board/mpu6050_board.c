/**
 * @file    mpu6050_board.c
 * @brief   MPU6050 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 用公共 device_iic_t 绑定默认 IIC 总线和器件地址（0x68）
 * - 通过 mpu6050_cfg_t.iic 将 I2C 传输契约注入 Device 层，消除 Device→Board 反向依赖
 * - 实例化 g_mpu6050 设备对象，绑定默认传感器配置
 * - 提供 MPU6050_Board_Init() 和 MPU6050_Board_GetDevice()
 *
 * 原先独立的 mpu6050_int.c 已合并到此文件。
 */

#include "mpu6050_board.h"
#include "iic_board.h"

#define MPU6050_INT_DEVICE_ADDR    (0x68U)

static mpu6050_t g_mpu6050;

static const mpu6050_cfg_t g_mpu6050_cfg = {
    .init_config = {
        .gyro_fs = MPU6050_GYRO_FS_2000DPS,
        .accel_fs = MPU6050_ACCEL_FS_16G,
        .dlpf_cfg = MPU6050_DLPF_5HZ,
        .sample_rate_div = 9U,
    },
    .iic = {
        .ops = &g_iic_board_default_ops,
        .context = (void *)0,
        .address = MPU6050_INT_DEVICE_ADDR,
    },
};

mpu6050_t *MPU6050_Board_GetDevice(void)
{
    return &g_mpu6050;
}

uint8_t MPU6050_Board_GetDeviceAddr(void)
{
    return MPU6050_INT_DEVICE_ADDR;
}

void MPU6050_Board_Init(void)
{
    MPU6050_InitDevice(&g_mpu6050, &g_mpu6050_cfg);
}
