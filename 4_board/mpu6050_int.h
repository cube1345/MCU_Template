#ifndef __MPU6050_INT_H
#define __MPU6050_INT_H

#include <stdint.h>

#include "bsp_iic.h"

/* MPU6050 当前挂载的 BSP IIC 端口，依据当前硬件连接固定为 IIC0。 */
#define MPU6050_INT_BSP_IIC_PORT          (BSP_IIC_PORT_0)

/* MPU6050 当前使用的 7 位 IIC 器件地址，依据原理图中 AD0 接地配置固定为 0x68。 */
#define MPU6050_INT_DEVICE_ADDR           (0x68U)

void mpu6050_int_init(void);
uint8_t mpu6050_int_get_device_addr(void);
bsp_iic_status_t mpu6050_int_check_device(void);
bsp_iic_status_t mpu6050_int_write_reg(uint8_t reg_addr, uint8_t reg_data);
bsp_iic_status_t mpu6050_int_read_reg(uint8_t reg_addr, uint8_t *reg_data);
bsp_iic_status_t mpu6050_int_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length);
bsp_iic_status_t mpu6050_int_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length);

#endif
