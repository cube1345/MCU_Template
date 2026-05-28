/**
 * @file    mpu6050_board.c
 * @brief   MPU6050 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 实现 mpu6050_i2c_ops_t 全部 7 个函数指针，绑定 I2C 端口（HAL_IIC_PORT_0）
 *   和器件地址（0x68）
 * - 通过 mpu6050_cfg_t.i2c 将 I2C 传输函数注入 Device 层，消除 Device→Board 反向依赖
 * - 实例化 g_mpu6050 设备对象，绑定默认传感器配置
 * - 提供 MPU6050_Board_Init() 和 MPU6050_Board_GetDevice()
 *
 * 原先独立的 mpu6050_int.c 已合并到此文件。
 */

#include "mpu6050_board.h"
#include "hal_iic.h"

/* ---- I2C 传输层（原 mpu6050_int.c） ------------------------------ */

#define MPU6050_INT_HAL_IIC_PORT   (HAL_IIC_PORT_0)
#define MPU6050_INT_DEVICE_ADDR    (0x68U)

static int32_t mpu6050_board_write_reg(uint8_t reg_addr, uint8_t reg_data)
{
    return (int32_t)hal_iic_write_reg8(
        MPU6050_INT_HAL_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr, reg_data);
}

static int32_t mpu6050_board_read_reg(uint8_t reg_addr, uint8_t *reg_data)
{
    return (int32_t)hal_iic_read_reg8(
        MPU6050_INT_HAL_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr, reg_data);
}

static int32_t mpu6050_board_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    return (int32_t)hal_iic_write_reg8s(
        MPU6050_INT_HAL_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr,
        write_data, write_length);
}

static int32_t mpu6050_board_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    return (int32_t)hal_iic_read_reg8s(
        MPU6050_INT_HAL_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr,
        read_data, read_length);
}

static void mpu6050_board_i2c_init(void)
{
    hal_iic_init();
}

static int32_t mpu6050_board_check_device(void)
{
    return (int32_t)hal_iic_check_device(
        MPU6050_INT_HAL_IIC_PORT, MPU6050_INT_DEVICE_ADDR);
}

static uint8_t mpu6050_board_get_device_addr(void)
{
    return MPU6050_INT_DEVICE_ADDR;
}

static const mpu6050_i2c_ops_t g_mpu6050_i2c_ops = {
    .write_reg = mpu6050_board_write_reg,
    .read_reg = mpu6050_board_read_reg,
    .write_regs = mpu6050_board_write_regs,
    .read_regs = mpu6050_board_read_regs,
    .init = mpu6050_board_i2c_init,
    .check_device = mpu6050_board_check_device,
    .get_device_addr = mpu6050_board_get_device_addr,
};

/* ---- 设备实例层 -------------------------------------------------- */

static mpu6050_t g_mpu6050;

static const mpu6050_cfg_t g_mpu6050_cfg = {
    .init_config = {
        .gyro_fs = MPU6050_GYRO_FS_2000DPS,
        .accel_fs = MPU6050_ACCEL_FS_16G,
        .dlpf_cfg = MPU6050_DLPF_5HZ,
        .sample_rate_div = 9U,
    },
    .i2c = &g_mpu6050_i2c_ops,
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
