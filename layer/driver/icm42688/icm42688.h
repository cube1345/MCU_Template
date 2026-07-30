/* SPDX-License-Identifier: MIT */
/**
 * @file    icm42688.h
 * @brief   ICM42688 六轴 IMU 的 I2C 驱动接口。
 * @author  weixin
 *
 * 本驱动只处理 ICM42688 芯片寄存器和物理量转换，不关心板级连线。
 * board 层负责传入已经初始化的 struct i2c 总线对象。
 */
#ifndef ICM42688_H
#define ICM42688_H

#include "imu/imu.h"
#include "platform/inferface/i2c/i2c.h"

#include <stdint.h>

/* 当前模块将 AD0/MISO 直接拉高，因此 I2C 地址固定为 0x69。 */
#define ICM42688_I2C_ADDR UINT8_C(0x69)

/* 陀螺仪量程配置宏，对应 GYRO_CONFIG0[7:5]。 */
#define ICM42688_GYRO_FS_2000DPS UINT8_C(0)
#define ICM42688_GYRO_FS_1000DPS UINT8_C(1)
#define ICM42688_GYRO_FS_500DPS UINT8_C(2)
#define ICM42688_GYRO_FS_250DPS UINT8_C(3)
#define ICM42688_GYRO_FS_125DPS UINT8_C(4)
#define ICM42688_GYRO_FS_62DPS5 UINT8_C(5)
#define ICM42688_GYRO_FS_31DPS25 UINT8_C(6)
#define ICM42688_GYRO_FS_15DPS625 UINT8_C(7)

/* 加速度计量程配置宏，对应 ACCEL_CONFIG0[7:5]。 */
#define ICM42688_ACCEL_FS_16G UINT8_C(0)
#define ICM42688_ACCEL_FS_8G UINT8_C(1)
#define ICM42688_ACCEL_FS_4G UINT8_C(2)
#define ICM42688_ACCEL_FS_2G UINT8_C(3)

/* 陀螺仪输出数据率配置宏，对应 GYRO_CONFIG0[3:0]。 */
#define ICM42688_GYRO_ODR_32KHZ UINT8_C(1)
#define ICM42688_GYRO_ODR_16KHZ UINT8_C(2)
#define ICM42688_GYRO_ODR_8KHZ UINT8_C(3)
#define ICM42688_GYRO_ODR_4KHZ UINT8_C(4)
#define ICM42688_GYRO_ODR_2KHZ UINT8_C(5)
#define ICM42688_GYRO_ODR_1KHZ UINT8_C(6)
#define ICM42688_GYRO_ODR_200HZ UINT8_C(7)
#define ICM42688_GYRO_ODR_100HZ UINT8_C(8)
#define ICM42688_GYRO_ODR_50HZ UINT8_C(9)
#define ICM42688_GYRO_ODR_25HZ UINT8_C(10)
#define ICM42688_GYRO_ODR_12HZ5 UINT8_C(11)
#define ICM42688_GYRO_ODR_500HZ UINT8_C(15)

/* 加速度计输出数据率配置宏，对应 ACCEL_CONFIG0[3:0]。 */
#define ICM42688_ACCEL_ODR_32KHZ UINT8_C(1)
#define ICM42688_ACCEL_ODR_16KHZ UINT8_C(2)
#define ICM42688_ACCEL_ODR_8KHZ UINT8_C(3)
#define ICM42688_ACCEL_ODR_4KHZ UINT8_C(4)
#define ICM42688_ACCEL_ODR_2KHZ UINT8_C(5)
#define ICM42688_ACCEL_ODR_1KHZ UINT8_C(6)
#define ICM42688_ACCEL_ODR_200HZ UINT8_C(7)
#define ICM42688_ACCEL_ODR_100HZ UINT8_C(8)
#define ICM42688_ACCEL_ODR_50HZ UINT8_C(9)
#define ICM42688_ACCEL_ODR_25HZ UINT8_C(10)
#define ICM42688_ACCEL_ODR_12HZ5 UINT8_C(11)
#define ICM42688_ACCEL_ODR_6HZ25 UINT8_C(12)
#define ICM42688_ACCEL_ODR_3HZ125 UINT8_C(13)
#define ICM42688_ACCEL_ODR_1HZ5625 UINT8_C(14)
#define ICM42688_ACCEL_ODR_500HZ UINT8_C(15)

struct icm42688_config {
  /* 陀螺仪量程选择，使用 ICM42688_GYRO_FS_* 宏。 */
  uint8_t gyro_fs;
  /* 陀螺仪输出数据率，使用 ICM42688_GYRO_ODR_* 宏。 */
  uint8_t gyro_odr;
  /* 加速度计量程选择，使用 ICM42688_ACCEL_FS_* 宏。 */
  uint8_t accel_fs;
  /* 加速度计输出数据率，使用 ICM42688_ACCEL_ODR_* 宏。 */
  uint8_t accel_odr;
};

struct icm42688_raw_data {
  /* 温度原始值。 */
  int16_t temperature;
  /* X 轴加速度原始值。 */
  int16_t accel_x;
  /* Y 轴加速度原始值。 */
  int16_t accel_y;
  /* Z 轴加速度原始值。 */
  int16_t accel_z;
  /* X 轴角速度原始值。 */
  int16_t gyro_x;
  /* Y 轴角速度原始值。 */
  int16_t gyro_y;
  /* Z 轴角速度原始值。 */
  int16_t gyro_z;
};

struct icm42688 {
  /* IMU 基类，必须放在第一个成员，便于 container_of 安全下转型。 */
  struct imu base;
  /* ICM42688 所挂载的 I2C 总线，由 board 层注入。 */
  struct i2c *bus;
  /* 7 位 I2C 设备地址；当前硬件固定为 ICM42688_I2C_ADDR。 */
  uint8_t dev_addr;
  /* 当前寄存器配置。 */
  struct icm42688_config config;
};

/**
 * @brief  获取 ICM42688 默认配置。
 * @param  config 用于接收默认配置的结构体。
 * @return PLATFORM_OK 表示成功；config 为 NULL 时返回参数错误。
 */
int icm42688_get_default_config(struct icm42688_config *config);

/**
 * @brief  初始化 ICM42688 驱动对象并绑定 IMU 基类。
 * @param  self 调用者提供的 ICM42688 对象，生命周期由调用者管理。
 * @param  name 板级逻辑名，通常为 "imu0"。
 * @param  bus 已初始化或可初始化的 I2C 总线对象。
 * @param  config ICM42688 配置；传 NULL 时使用默认配置。
 * @return PLATFORM_OK 表示对象绑定成功，其他值表示参数或状态错误。
 * @note   本函数不立即访问芯片，真正硬件初始化在 imu_start() 中完成。
 */
int icm42688_init(struct icm42688 *self,
                  const char *name,
                  struct i2c *bus,
                  const struct icm42688_config *config);

/**
 * @brief  获取 ICM42688 对应的 IMU 基类指针。
 * @param  self ICM42688 驱动对象。
 * @return struct imu * 基类指针；self 为 NULL 时返回 NULL。
 */
struct imu *icm42688_get_imu(struct icm42688 *self);

#endif /* ICM42688_H */
