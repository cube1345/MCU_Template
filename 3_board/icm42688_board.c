/**
 * @file    icm42688_board.c
 * @brief   ICM42688 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 实现 icm42688_i2c_ops_t 全部 7 个函数指针，绑定 I2C 端口（HAL_IIC_PORT_0）
 *   和器件地址（0x69）
 * - 通过 icm42688_cfg_t.i2c 将 I2C 传输函数注入 Device 层，消除 Device→Board 反向依赖
 * - 实例化 g_icm42688 设备对象，绑定默认传感器配置
 * - 提供 ICM42688_Board_Init() 和 ICM42688_Board_GetDevice()
 *
 * 原先独立的 icm42688_int.c 已合并到此文件。
 */

#include "icm42688_board.h"
#include "hal_iic.h"

/* ---- I2C 传输层（原 icm42688_int.c） ---------------------------- */

#define ICM42688_INT_HAL_IIC_PORT   (HAL_IIC_PORT_0)
#define ICM42688_INT_DEVICE_ADDR    (0x69U)

static int32_t icm42688_board_write_reg(uint8_t reg_addr, uint8_t reg_data)
{
    return (int32_t)hal_iic_write_reg8(
        ICM42688_INT_HAL_IIC_PORT, ICM42688_INT_DEVICE_ADDR, reg_addr, reg_data);
}

static int32_t icm42688_board_read_reg(uint8_t reg_addr, uint8_t *reg_data)
{
    return (int32_t)hal_iic_read_reg8(
        ICM42688_INT_HAL_IIC_PORT, ICM42688_INT_DEVICE_ADDR, reg_addr, reg_data);
}

static int32_t icm42688_board_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    return (int32_t)hal_iic_write_reg8s(
        ICM42688_INT_HAL_IIC_PORT, ICM42688_INT_DEVICE_ADDR, reg_addr,
        write_data, write_length);
}

static int32_t icm42688_board_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    return (int32_t)hal_iic_read_reg8s(
        ICM42688_INT_HAL_IIC_PORT, ICM42688_INT_DEVICE_ADDR, reg_addr,
        read_data, read_length);
}

static void icm42688_board_i2c_init(void)
{
    hal_iic_init();
}

static int32_t icm42688_board_check_device(void)
{
    return (int32_t)hal_iic_check_device(
        ICM42688_INT_HAL_IIC_PORT, ICM42688_INT_DEVICE_ADDR);
}

static uint8_t icm42688_board_get_device_addr(void)
{
    return ICM42688_INT_DEVICE_ADDR;
}

static const icm42688_i2c_ops_t g_icm42688_i2c_ops = {
    .write_reg = icm42688_board_write_reg,
    .read_reg = icm42688_board_read_reg,
    .write_regs = icm42688_board_write_regs,
    .read_regs = icm42688_board_read_regs,
    .init = icm42688_board_i2c_init,
    .check_device = icm42688_board_check_device,
    .get_device_addr = icm42688_board_get_device_addr,
};

/* ---- 设备实例层 -------------------------------------------------- */

static icm42688_t g_icm42688;

static const icm42688_cfg_t g_icm42688_cfg = {
    .init_config = {
        .gyro_fs = ICM42688_GYRO_FS_2000DPS,
        .gyro_odr = ICM42688_GYRO_ODR_1KHZ,
        .accel_fs = ICM42688_ACCEL_FS_4G,
        .accel_odr = ICM42688_ACCEL_ODR_1KHZ,
    },
    .i2c = &g_icm42688_i2c_ops,
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
