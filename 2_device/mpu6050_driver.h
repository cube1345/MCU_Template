#ifndef MYUSED_MYDEVICES_MPU6050_DRIVER_H
#define MYUSED_MYDEVICES_MPU6050_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "device_base.h"
#include "device_ops.h"

/* MPU6050 采样率分频寄存器地址。 */
#define MPU6050_REG_SMPLRT_DIV                (0x19U)

/* MPU6050 数字低通滤波器配置寄存器地址。 */
#define MPU6050_REG_CONFIG                    (0x1AU)

/* MPU6050 陀螺仪配置寄存器地址。 */
#define MPU6050_REG_GYRO_CONFIG               (0x1BU)

/* MPU6050 加速度计配置寄存器地址。 */
#define MPU6050_REG_ACCEL_CONFIG              (0x1CU)

/* MPU6050 加速度计 X 轴高字节寄存器地址，连续读原始数据从这里开始。 */
#define MPU6050_REG_ACCEL_XOUT_H              (0x3BU)

/* MPU6050 电源管理 1 寄存器地址。 */
#define MPU6050_REG_PWR_MGMT_1                (0x6BU)

/* MPU6050 WHO_AM_I 寄存器地址。 */
#define MPU6050_REG_WHO_AM_I                  (0x75U)

/* MPU6050 正确的 WHO_AM_I 返回值。 */
#define MPU6050_WHO_AM_I_VALUE                (0x68U)

/* MPU6050 从休眠唤醒的控制值（清除 SLEEP 位）。 */
#define MPU6050_PWR_MGMT_1_WAKEUP             (0x00U)

/* 一次连续读取加速度计、温度和陀螺仪原始数据的总字节数。 */
#define MPU6050_RAW_DATA_LENGTH               (14U)

/* 传感器启动后的稳定等待时间，单位毫秒。 */
#define MPU6050_SENSOR_START_DELAY_MS         (50U)

/* 角度转弧度换算系数，单位 rad/deg。 */
#define MPU6050_DEG_TO_RAD                    (0.01745329251994329577f)

/**
  * @brief  MPU6050 驱动层统一状态码定义。
  */
typedef enum
{
    MPU6050_STATUS_OK = 0U,
    MPU6050_STATUS_ERROR_PARAM,
    MPU6050_STATUS_ERROR_COMM,
    MPU6050_STATUS_ERROR_ID
} mpu6050_status_t;

/**
  * @brief  MPU6050 陀螺仪量程枚举定义，对应 GYRO_CONFIG[4:3]。
  */
typedef enum
{
    MPU6050_GYRO_FS_250DPS = 0U,
    MPU6050_GYRO_FS_500DPS = 1U,
    MPU6050_GYRO_FS_1000DPS = 2U,
    MPU6050_GYRO_FS_2000DPS = 3U
} mpu6050_gyro_fs_t;

/**
  * @brief  MPU6050 加速度计量程枚举定义，对应 ACCEL_CONFIG[4:3]。
  */
typedef enum
{
    MPU6050_ACCEL_FS_2G = 0U,
    MPU6050_ACCEL_FS_4G = 1U,
    MPU6050_ACCEL_FS_8G = 2U,
    MPU6050_ACCEL_FS_16G = 3U
} mpu6050_accel_fs_t;

/**
  * @brief  MPU6050 数字低通滤波器带宽枚举定义，对应 CONFIG[2:0]。
  */
typedef enum
{
    MPU6050_DLPF_260HZ = 0U,
    MPU6050_DLPF_184HZ = 1U,
    MPU6050_DLPF_94HZ = 2U,
    MPU6050_DLPF_44HZ = 3U,
    MPU6050_DLPF_21HZ = 4U,
    MPU6050_DLPF_10HZ = 5U,
    MPU6050_DLPF_5HZ = 6U
} mpu6050_dlpf_cfg_t;

/**
  * @brief  MPU6050 初始化配置结构体。
  */
typedef struct
{
    mpu6050_gyro_fs_t gyro_fs;
    mpu6050_accel_fs_t accel_fs;
    mpu6050_dlpf_cfg_t dlpf_cfg;
    uint8_t sample_rate_div;
} mpu6050_init_config_t;

/**
  * @brief  MPU6050 原始数据结构体。
  */
typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temperature;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6050_raw_data_t;

/**
  * @brief  MPU6050 换算后的物理量数据结构体。
  */
typedef struct
{
    float temperature_c;
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float gyro_x_rad_s;
    float gyro_y_rad_s;
    float gyro_z_rad_s;
} mpu6050_scaled_data_t;

typedef struct mpu6050_s mpu6050_t;
typedef device_sensor_ops_t mpu6050_base_ops_t;

/**
  * @brief I2C 传输操作函数指针表，由 Board 层实现并注入 Device 层。
  *
  * 与 ICM42688 共用相同的注入模式。Board 层在 *_board.c 中实现这些函数，
  * 通过 mpu6050_cfg_t.i2c 注入，Device 层不再反向 include Board 层头文件。
  */
typedef struct {
    int32_t (*write_reg)(uint8_t reg_addr, uint8_t reg_data);
    int32_t (*read_reg)(uint8_t reg_addr, uint8_t *reg_data);
    int32_t (*write_regs)(uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);
    int32_t (*read_regs)(uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);
    void    (*init)(void);
    int32_t (*check_device)(void);
    uint8_t (*get_device_addr)(void);
} mpu6050_i2c_ops_t;

typedef struct {
    mpu6050_init_config_t init_config;
    const mpu6050_i2c_ops_t *i2c;
} mpu6050_cfg_t;

struct mpu6050_s {
    device_base_t base;
    const mpu6050_base_ops_t *ops;
    mpu6050_cfg_t cfg;
};

void MPU6050_InitDevice(mpu6050_t *me, const mpu6050_cfg_t *cfg);
int32_t MPU6050_ReadRawDevice(mpu6050_t *me, mpu6050_raw_data_t *raw_data);
int32_t MPU6050_ReadScaledDevice(mpu6050_t *me, mpu6050_scaled_data_t *scaled_data);

mpu6050_status_t mpu6050_get_default_init_config(mpu6050_init_config_t *init_config);
mpu6050_status_t mpu6050_get_current_config(mpu6050_init_config_t *init_config);
mpu6050_status_t mpu6050_init(void);
mpu6050_status_t mpu6050_init_with_config(const mpu6050_init_config_t *init_config);
mpu6050_status_t mpu6050_init_default(void);
mpu6050_status_t mpu6050_set_gyro_config(mpu6050_gyro_fs_t gyro_fs);
mpu6050_status_t mpu6050_set_accel_config(mpu6050_accel_fs_t accel_fs);
mpu6050_status_t mpu6050_check_id(void);
mpu6050_status_t mpu6050_get_who_am_i(uint8_t *who_am_i);
mpu6050_status_t mpu6050_write_reg(uint8_t reg_addr, uint8_t reg_data);
mpu6050_status_t mpu6050_read_reg(uint8_t reg_addr, uint8_t *reg_data);
mpu6050_status_t mpu6050_write_regs(uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);
mpu6050_status_t mpu6050_read_regs(uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);
mpu6050_status_t mpu6050_read_raw_data(mpu6050_raw_data_t *raw_data);

const char *mpu6050_get_gyro_fs_string(mpu6050_gyro_fs_t gyro_fs);
const char *mpu6050_get_accel_fs_string(mpu6050_accel_fs_t accel_fs);
const char *mpu6050_get_dlpf_string(mpu6050_dlpf_cfg_t dlpf_cfg);
float mpu6050_get_accel_lsb_per_g(mpu6050_accel_fs_t accel_fs);
float mpu6050_get_gyro_lsb_per_dps(mpu6050_gyro_fs_t gyro_fs);

mpu6050_status_t mpu6050_convert_raw_data(
    const mpu6050_init_config_t *init_config,
    const mpu6050_raw_data_t *raw_data,
    mpu6050_scaled_data_t *scaled_data);

#endif /* MYUSED_MYDEVICES_MPU6050_DRIVER_H */
