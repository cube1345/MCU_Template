#include "mpu6050_int.h"

void mpu6050_int_init(void)
{
    bsp_iic_init();
}

uint8_t mpu6050_int_get_device_addr(void)
{
    return MPU6050_INT_DEVICE_ADDR;
}

bsp_iic_status_t mpu6050_int_check_device(void)
{
    return bsp_iic_check_device(MPU6050_INT_BSP_IIC_PORT, MPU6050_INT_DEVICE_ADDR);
}

bsp_iic_status_t mpu6050_int_write_reg(uint8_t reg_addr, uint8_t reg_data)
{
    return bsp_iic_write_reg8(
        MPU6050_INT_BSP_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr, reg_data);
}

bsp_iic_status_t mpu6050_int_read_reg(uint8_t reg_addr, uint8_t *reg_data)
{
    return bsp_iic_read_reg8(
        MPU6050_INT_BSP_IIC_PORT, MPU6050_INT_DEVICE_ADDR, reg_addr, reg_data);
}

bsp_iic_status_t mpu6050_int_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    return bsp_iic_write_reg8s(MPU6050_INT_BSP_IIC_PORT,
        MPU6050_INT_DEVICE_ADDR, reg_addr, write_data, write_length);
}

bsp_iic_status_t mpu6050_int_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    return bsp_iic_read_reg8s(MPU6050_INT_BSP_IIC_PORT,
        MPU6050_INT_DEVICE_ADDR, reg_addr, read_data, read_length);
}
