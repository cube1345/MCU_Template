/**
 * @file icm42688_board.c
 * @brief 将 ICM42688 对象绑定到 CubeMX 的 I2C2。
 * @details 仅注入硬件资源。0x68 为板级配置假设，尚未通过实物确认。
 * @author cube
 * @date 2026-09-13
 * @note Updated: 2026-09-13
 */
#include "icm42688_board.h"

#include "i2c.h"
#include "hal_iic.h"

static hal_iic_t g_icm42688_bus = {
    .cfg = {
        .handle = &hi2c2,
        .timeout_ms = 100U,
    },
};

static icm42688_t g_icm42688;

/** @brief 将设备实例的单寄存器写请求转交给注入的总线。 */
static int32_t ICM42688_Board_WriteReg(
    void *context, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data)
{
    return (int32_t)hal_iic_write_reg8(
        (hal_iic_t *)context, device_addr, reg_addr, reg_data);
}

/** @brief 将设备实例的单寄存器读请求转交给注入的总线。 */
static int32_t ICM42688_Board_ReadReg(
    void *context, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data)
{
    return (int32_t)hal_iic_read_reg8(
        (hal_iic_t *)context, device_addr, reg_addr, reg_data);
}

/** @brief 转发阻塞式连续寄存器写请求。 */
static int32_t ICM42688_Board_WriteRegs(
    void *context, uint8_t device_addr, uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    return (int32_t)hal_iic_write_reg8s(
        (hal_iic_t *)context, device_addr, reg_addr, write_data, write_length);
}

/** @brief 转发阻塞式连续寄存器读请求。 */
static int32_t ICM42688_Board_ReadRegs(
    void *context, uint8_t device_addr, uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    return (int32_t)hal_iic_read_reg8s(
        (hal_iic_t *)context, device_addr, reg_addr, read_data, read_length);
}

/** @brief 检查当前总线上指定 7 位地址的设备应答。 */
static int32_t ICM42688_Board_CheckDevice(void *context, uint8_t device_addr)
{
    return (int32_t)hal_iic_check_device(
        (hal_iic_t *)context, device_addr);
}

/** @brief 初始化总线封装状态；CubeMX 必须提前初始化硬件句柄。 */
static void ICM42688_Board_I2CInit(void *context)
{
    hal_iic_init((hal_iic_t *)context);
}

static const icm42688_i2c_ops_t g_icm42688_i2c_ops = {
    .write_reg = ICM42688_Board_WriteReg,
    .read_reg = ICM42688_Board_ReadReg,
    .write_regs = ICM42688_Board_WriteRegs,
    .read_regs = ICM42688_Board_ReadRegs,
    .init = ICM42688_Board_I2CInit,
    .check_device = ICM42688_Board_CheckDevice,
};

static const icm42688_cfg_t g_icm42688_cfg = {
    .init_config = {
        .gyro_fs = ICM42688_GYRO_FS_2000DPS,
        .gyro_odr = ICM42688_GYRO_ODR_1KHZ,
        .accel_fs = ICM42688_ACCEL_FS_4G,
        .accel_odr = ICM42688_ACCEL_ODR_1KHZ,
    },
    .i2c = &g_icm42688_i2c_ops,
    .i2c_context = &g_icm42688_bus,
    .device_addr = 0x68U,
    .delay_ms = hal_iic_delay_ms,
    .delay_context = (void *)0,
};

void ICM42688_Board_Init(void)
{
    ICM42688_InitDevice(&g_icm42688, &g_icm42688_cfg);
}

icm42688_t *ICM42688_Board_GetDevice(void)
{
    return &g_icm42688;
}

uint8_t ICM42688_Board_GetDeviceAddr(void)
{
    return g_icm42688.cfg.device_addr;
}
