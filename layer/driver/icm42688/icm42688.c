/* SPDX-License-Identifier: MIT */
/**
 * @file    icm42688.c
 * @brief   ICM42688 寄存器初始化、采样和物理量转换。
 * @author  weixin
 */
#include "icm42688/icm42688.h"

#include "container_of.h"
#include "platform/inferface/i2c/i2c_blocking.h"
#include "platform/inferface/timebase/timebase.h"

#include <stddef.h>

/* ICM42688 设备配置寄存器，用于触发软件复位。 */
#define ICM42688_REG_DEVICE_CONFIG UINT8_C(0x11)
/* 温度、加速度、陀螺仪连续输出数据的起始寄存器。 */
#define ICM42688_REG_TEMP_DATA1 UINT8_C(0x1D)
/* 传感器电源管理寄存器。 */
#define ICM42688_REG_PWR_MGMT0 UINT8_C(0x4E)
/* 陀螺仪量程和输出数据率配置寄存器。 */
#define ICM42688_REG_GYRO_CONFIG0 UINT8_C(0x4F)
/* 加速度计量程和输出数据率配置寄存器。 */
#define ICM42688_REG_ACCEL_CONFIG0 UINT8_C(0x50)
/* 芯片身份识别寄存器。 */
#define ICM42688_REG_WHO_AM_I UINT8_C(0x75)

/* ICM42688-P 的 WHO_AM_I 固定返回值。 */
#define ICM42688_WHO_AM_I_VALUE UINT8_C(0x47)
/* 写入 DEVICE_CONFIG 后触发软件复位的位掩码。 */
#define ICM42688_SOFT_RESET_MASK UINT8_C(0x01)
/* 加速度计和陀螺仪同时进入低噪声模式的配置值。 */
#define ICM42688_PWR_MGMT0_ACCEL_GYRO_LN UINT8_C(0x0F)
/* 一次突发读取温度、三轴加速度和三轴陀螺仪所需字节数。 */
#define ICM42688_RAW_DATA_LENGTH UINT16_C(14)
/* 软件复位后等待芯片重新响应的时间，单位 ms。 */
#define ICM42688_RESET_DELAY_MS UINT32_C(10)
/* 传感器上电并进入低噪声模式后的稳定等待时间，单位 ms。 */
#define ICM42688_SENSOR_START_DELAY_MS UINT32_C(50)
/* ICM42688 使用 8 位寄存器地址。 */
#define ICM42688_REG_ADDR_SIZE PLATFORM_I2C_MEM_ADDR_SIZE_8BIT

/**
 * @brief  判断 ICM42688 I2C 地址是否合法。
 * @param  address 待检查的 7 位 I2C 地址。
 * @return 1 表示合法，0 表示非法。
 */
static int icm42688_address_is_valid(uint8_t address)
{
  return (address <= UINT8_C(0x7F)) ? 1 : 0;
}

/**
 * @brief  合并寄存器高低字节为有符号 16 位原始值。
 * @param  high_byte 高字节。
 * @param  low_byte 低字节。
 * @return 合并后的 int16_t 原始值。
 */
static int16_t icm42688_merge_bytes(uint8_t high_byte, uint8_t low_byte)
{
  return (int16_t)(((uint16_t)high_byte << 8U) | (uint16_t)low_byte);
}

/**
 * @brief  生成 GYRO_CONFIG0 寄存器值。
 * @param  gyro_fs 陀螺仪量程宏值。
 * @param  gyro_odr 陀螺仪输出数据率宏值。
 * @return 可直接写入 GYRO_CONFIG0 的寄存器值。
 */
static uint8_t icm42688_build_gyro_config0(uint8_t gyro_fs,
                                           uint8_t gyro_odr)
{
  return (uint8_t)((gyro_fs << 5U) | (gyro_odr & UINT8_C(0x0F)));
}

/**
 * @brief  生成 ACCEL_CONFIG0 寄存器值。
 * @param  accel_fs 加速度计量程宏值。
 * @param  accel_odr 加速度计输出数据率宏值。
 * @return 可直接写入 ACCEL_CONFIG0 的寄存器值。
 */
static uint8_t icm42688_build_accel_config0(uint8_t accel_fs,
                                            uint8_t accel_odr)
{
  return (uint8_t)((accel_fs << 5U) | (accel_odr & UINT8_C(0x0F)));
}

/**
 * @brief  获取当前加速度量程下每 g 对应的 LSB。
 * @param  accel_fs 加速度计量程宏值。
 * @return 每 g 的 LSB 数；量程非法时返回 0。
 */
static float icm42688_accel_lsb_per_g(uint8_t accel_fs)
{
  switch (accel_fs) {
  case ICM42688_ACCEL_FS_16G:
    return 2048.0f;
  case ICM42688_ACCEL_FS_8G:
    return 4096.0f;
  case ICM42688_ACCEL_FS_4G:
    return 8192.0f;
  case ICM42688_ACCEL_FS_2G:
    return 16384.0f;
  default:
    return 0.0f;
  }
}

/**
 * @brief  获取当前陀螺仪量程下每 deg/s 对应的 LSB。
 * @param  gyro_fs 陀螺仪量程宏值。
 * @return 每 deg/s 的 LSB 数；量程非法时返回 0。
 */
static float icm42688_gyro_lsb_per_dps(uint8_t gyro_fs)
{
  switch (gyro_fs) {
  case ICM42688_GYRO_FS_2000DPS:
    return 16.4f;
  case ICM42688_GYRO_FS_1000DPS:
    return 32.8f;
  case ICM42688_GYRO_FS_500DPS:
    return 65.5f;
  case ICM42688_GYRO_FS_250DPS:
    return 131.0f;
  case ICM42688_GYRO_FS_125DPS:
    return 262.0f;
  case ICM42688_GYRO_FS_62DPS5:
    return 524.3f;
  case ICM42688_GYRO_FS_31DPS25:
    return 1048.6f;
  case ICM42688_GYRO_FS_15DPS625:
    return 2097.2f;
  default:
    return 0.0f;
  }
}

/**
 * @brief  读取 ICM42688 连续寄存器。
 * @param  self ICM42688 驱动对象。
 * @param  reg_addr 起始寄存器地址。
 * @param  data 接收缓冲区。
 * @param  size 读取字节数。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_reg_read(struct icm42688 *self,
                             uint8_t reg_addr,
                             uint8_t *data,
                             uint16_t size)
{
  struct i2c_mem_address address;

  if ((self == NULL) || (self->bus == NULL) || (data == NULL) ||
      (size == 0U) || !icm42688_address_is_valid(self->dev_addr)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  address.device_addr = self->dev_addr;
  address.memory_addr = reg_addr;
  address.memory_addr_size = ICM42688_REG_ADDR_SIZE;

  return i2c_mem_read(self->bus, &address, data, size);
}

/**
 * @brief  写入 ICM42688 单个寄存器。
 * @param  self ICM42688 驱动对象。
 * @param  reg_addr 寄存器地址。
 * @param  data 待写入的寄存器值。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_reg_write(struct icm42688 *self,
                              uint8_t reg_addr,
                              uint8_t data)
{
  struct i2c_mem_address address;

  if ((self == NULL) || (self->bus == NULL) ||
      !icm42688_address_is_valid(self->dev_addr)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  address.device_addr = self->dev_addr;
  address.memory_addr = reg_addr;
  address.memory_addr_size = ICM42688_REG_ADDR_SIZE;

  return i2c_mem_write(self->bus, &address, &data, 1U);
}

/**
 * @brief  读取并校验 ICM42688 WHO_AM_I。
 * @param  self ICM42688 驱动对象。
 * @return PLATFORM_OK 表示芯片 ID 正确，其他 PLATFORM_* 表示失败。
 */
static int icm42688_check_id(struct icm42688 *self)
{
  uint8_t who_am_i;
  int result;

  result = icm42688_reg_read(self, ICM42688_REG_WHO_AM_I,
                             &who_am_i, 1U);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (who_am_i != ICM42688_WHO_AM_I_VALUE) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  return PLATFORM_OK;
}

/**
 * @brief  对 ICM42688 执行软件复位并重新校验 ID。
 * @param  self ICM42688 驱动对象。
 * @return PLATFORM_OK 表示复位成功，其他 PLATFORM_* 表示失败。
 * @note   软件复位后芯片需要短暂恢复时间，因此延时后再读取 WHO_AM_I。
 */
static int icm42688_reset_chip(struct icm42688 *self)
{
  int result;

  result = icm42688_reg_write(self, ICM42688_REG_DEVICE_CONFIG,
                              ICM42688_SOFT_RESET_MASK);
  if (result != PLATFORM_OK) {
    return result;
  }

  delay_ms(ICM42688_RESET_DELAY_MS);
  return icm42688_check_id(self);
}

/**
 * @brief  初始化 ICM42688 芯片寄存器。
 * @param  self ICM42688 驱动对象。
 * @return PLATFORM_OK 表示初始化成功，其他 PLATFORM_* 表示失败。
 * @note   当前硬件将 AD0/MISO 固定拉高，因此驱动只使用 0x69 地址。
 */
static int icm42688_init_chip(struct icm42688 *self)
{
  uint8_t gyro_config0;
  uint8_t accel_config0;
  int result;

  if ((self == NULL) || (self->bus == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  result = icm42688_check_id(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  result = icm42688_reset_chip(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  gyro_config0 = icm42688_build_gyro_config0(self->config.gyro_fs,
                                             self->config.gyro_odr);
  accel_config0 = icm42688_build_accel_config0(self->config.accel_fs,
                                               self->config.accel_odr);

  result = icm42688_reg_write(self, ICM42688_REG_GYRO_CONFIG0,
                              gyro_config0);
  if (result != PLATFORM_OK) {
    return result;
  }

  result = icm42688_reg_write(self, ICM42688_REG_ACCEL_CONFIG0,
                              accel_config0);
  if (result != PLATFORM_OK) {
    return result;
  }

  return icm42688_reg_write(self, ICM42688_REG_PWR_MGMT0,
                            ICM42688_PWR_MGMT0_ACCEL_GYRO_LN);
}

/**
 * @brief  读取 ICM42688 温度、加速度和陀螺仪原始数据。
 * @param  self ICM42688 驱动对象。
 * @param  raw_data 用于接收原始数据的结构体。
 * @return PLATFORM_OK 表示读取成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_read_raw_data(struct icm42688 *self,
                                  struct icm42688_raw_data *raw_data)
{
  uint8_t raw_buffer[ICM42688_RAW_DATA_LENGTH];
  int result;

  if (raw_data == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  result = icm42688_reg_read(self, ICM42688_REG_TEMP_DATA1,
                             raw_buffer, ICM42688_RAW_DATA_LENGTH);
  if (result != PLATFORM_OK) {
    return result;
  }

  raw_data->temperature = icm42688_merge_bytes(raw_buffer[0], raw_buffer[1]);
  raw_data->accel_x = icm42688_merge_bytes(raw_buffer[2], raw_buffer[3]);
  raw_data->accel_y = icm42688_merge_bytes(raw_buffer[4], raw_buffer[5]);
  raw_data->accel_z = icm42688_merge_bytes(raw_buffer[6], raw_buffer[7]);
  raw_data->gyro_x = icm42688_merge_bytes(raw_buffer[8], raw_buffer[9]);
  raw_data->gyro_y = icm42688_merge_bytes(raw_buffer[10], raw_buffer[11]);
  raw_data->gyro_z = icm42688_merge_bytes(raw_buffer[12], raw_buffer[13]);

  return PLATFORM_OK;
}

/**
 * @brief  将 ICM42688 原始值转换为 IMU 标准物理量。
 * @param  config 当前 ICM42688 量程配置。
 * @param  raw_data ICM42688 原始采样值。
 * @param  data 用于接收转换结果的 IMU 数据结构。
 * @return PLATFORM_OK 表示转换成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_convert_raw_data(const struct icm42688_config *config,
                                     const struct icm42688_raw_data *raw_data,
                                     struct imu_data *data)
{
  float accel_lsb_per_g;
  float gyro_lsb_per_dps;

  if ((config == NULL) || (raw_data == NULL) || (data == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  accel_lsb_per_g = icm42688_accel_lsb_per_g(config->accel_fs);
  gyro_lsb_per_dps = icm42688_gyro_lsb_per_dps(config->gyro_fs);
  if ((accel_lsb_per_g <= 0.0f) || (gyro_lsb_per_dps <= 0.0f)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  data->temperature_c = ((float)raw_data->temperature / 132.48f) + 25.0f;
  data->accel_g.x = (float)raw_data->accel_x / accel_lsb_per_g;
  data->accel_g.y = (float)raw_data->accel_y / accel_lsb_per_g;
  data->accel_g.z = (float)raw_data->accel_z / accel_lsb_per_g;
  data->gyro_dps.x = (float)raw_data->gyro_x / gyro_lsb_per_dps;
  data->gyro_dps.y = (float)raw_data->gyro_y / gyro_lsb_per_dps;
  data->gyro_dps.z = (float)raw_data->gyro_z / gyro_lsb_per_dps;
  data->gyro_rad_s.x = data->gyro_dps.x * IMU_DEG_TO_RAD;
  data->gyro_rad_s.y = data->gyro_dps.y * IMU_DEG_TO_RAD;
  data->gyro_rad_s.z = data->gyro_dps.z * IMU_DEG_TO_RAD;

  return PLATFORM_OK;
}

/**
 * @brief  IMU 基类 start 操作的 ICM42688 实现。
 * @param  base IMU 基类对象。
 * @return PLATFORM_OK 表示启动成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_start(struct imu *base)
{
  struct icm42688 *self;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  self = container_of(base, struct icm42688, base);
  result = icm42688_init_chip(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  delay_ms(ICM42688_SENSOR_START_DELAY_MS);
  return PLATFORM_OK;
}

/**
 * @brief  IMU 基类 stop 操作的 ICM42688 实现。
 * @param  base IMU 基类对象，当前未使用。
 * @return PLATFORM_OK 表示停止成功。
 * @note   当前实现不关闭芯片电源，仅满足抽象生命周期接口。
 */
static int icm42688_stop(struct imu *base)
{
  (void)base;
  return PLATFORM_OK;
}

/**
 * @brief  IMU 基类 read 操作的 ICM42688 实现。
 * @param  base IMU 基类对象。
 * @return PLATFORM_OK 表示采样成功，其他 PLATFORM_* 表示失败。
 */
static int icm42688_read(struct imu *base)
{
  struct icm42688 *self;
  struct icm42688_raw_data raw_data;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  self = container_of(base, struct icm42688, base);
  result = icm42688_read_raw_data(self, &raw_data);
  if (result != PLATFORM_OK) {
    return result;
  }

  return icm42688_convert_raw_data(&self->config, &raw_data, &base->data);
}

/* ICM42688 绑定到 IMU 抽象层的操作表。 */
static const struct imu_ops s_icm42688_ops = {
  .start = icm42688_start,
  .stop = icm42688_stop,
  .read = icm42688_read,
};

/**
 * @brief  获取 ICM42688 默认配置。
 * @param  config 用于接收默认配置的结构体。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示失败。
 */
int icm42688_get_default_config(struct icm42688_config *config)
{
  if (config == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  config->gyro_fs = ICM42688_GYRO_FS_2000DPS;
  config->gyro_odr = ICM42688_GYRO_ODR_1KHZ;
  config->accel_fs = ICM42688_ACCEL_FS_4G;
  config->accel_odr = ICM42688_ACCEL_ODR_1KHZ;

  return PLATFORM_OK;
}

/**
 * @brief  初始化 ICM42688 驱动对象并绑定 IMU 基类。
 * @param  self 调用者提供的 ICM42688 对象。
 * @param  name 板级逻辑名，生命周期必须长于 self。
 * @param  bus ICM42688 所挂载的 I2C 总线对象。
 * @param  config 芯片配置；传 NULL 时使用默认配置。
 * @return PLATFORM_OK 表示绑定成功，其他 PLATFORM_* 表示失败。
 */
int icm42688_init(struct icm42688 *self,
                  const char *name,
                  struct i2c *bus,
                  const struct icm42688_config *config)
{
  struct icm42688_config default_config;
  int result;

  if ((self == NULL) || (name == NULL) || (bus == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  if (config == NULL) {
    result = icm42688_get_default_config(&default_config);
    if (result != PLATFORM_OK) {
      return result;
    }
    config = &default_config;
  }

  self->bus = bus;
  self->dev_addr = ICM42688_I2C_ADDR;
  self->config = *config;

  return imu_bind(&self->base, name, "ICM42688", &s_icm42688_ops);
}

/**
 * @brief  获取 ICM42688 对应的 IMU 基类对象。
 * @param  self ICM42688 驱动对象。
 * @return IMU 基类指针；self 为 NULL 时返回 NULL。
 */
struct imu *icm42688_get_imu(struct icm42688 *self)
{
  if (self == NULL) {
    return NULL;
  }

  return &self->base;
}
