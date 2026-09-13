#include "mpu6050_driver.h"

#include <string.h>


/** @brief Transport binding is independent of hardware initialization. */
static bool mpu6050_is_bound(const mpu6050_t *me)
{
    return me && me->cfg.i2c &&
        (me->cfg.device_addr == 0x68U || me->cfg.device_addr == 0x69U);
}

static int32_t MPU6050_Base_Init(void *me)
{
    mpu6050_t *dev = (mpu6050_t *)me;
    if (dev == (mpu6050_t *)0) {
        return (int32_t)MPU6050_STATUS_ERROR_PARAM;
    }
    return (int32_t)mpu6050_init_with_config(dev, &dev->cfg.init_config);
}

static int32_t MPU6050_Base_ReadRaw(void *me, void *raw_data)
{
    return (int32_t)MPU6050_ReadRawDevice((mpu6050_t *)me, (mpu6050_raw_data_t *)raw_data);
}

static int32_t MPU6050_Base_ReadScaled(void *me, void *scaled_data)
{
    return (int32_t)MPU6050_ReadScaledDevice((mpu6050_t *)me, (mpu6050_scaled_data_t *)scaled_data);
}

static const mpu6050_base_ops_t g_mpu6050_base_ops = {
    .init = MPU6050_Base_Init,
    .read_raw = MPU6050_Base_ReadRaw,
    .read_scaled = MPU6050_Base_ReadScaled,
};

static inline const mpu6050_base_ops_t *MPU6050_GetBaseOps(void)
{
    return &g_mpu6050_base_ops;
}

void MPU6050_InitDevice(mpu6050_t *me, const mpu6050_cfg_t *cfg)
{
    if (me == NULL) { return; }
    mpu6050_cfg_t copy = {0};
    if (cfg != NULL) { copy = *cfg; }
    memset(me, 0, sizeof(*me));
    me->ops = MPU6050_GetBaseOps();
    me->cfg = copy;
    /* Construction binds the transport; ops->init initializes hardware. */

}

int32_t MPU6050_ReadRawDevice(mpu6050_t *me, mpu6050_raw_data_t *raw_data)
{
    if ((me == (mpu6050_t *)0) || (me->base.is_init == 0U)) {
        return (int32_t)MPU6050_STATUS_ERROR_PARAM;
    }
    return (int32_t)mpu6050_read_raw_data(me, raw_data);
}

int32_t MPU6050_ReadScaledDevice(mpu6050_t *me, mpu6050_scaled_data_t *scaled_data)
{
    mpu6050_raw_data_t raw_data;
    mpu6050_status_t status;

    if ((me == (mpu6050_t *)0) || (scaled_data == (mpu6050_scaled_data_t *)0) ||
        (me->base.is_init == 0U)) {
        return (int32_t)MPU6050_STATUS_ERROR_PARAM;
    }

    status = mpu6050_read_raw_data(me, &raw_data);
    if (status != MPU6050_STATUS_OK) {
        return (int32_t)status;
    }

    return (int32_t)mpu6050_convert_raw_data(&me->current_config, &raw_data, scaled_data);
}

static mpu6050_status_t mpu6050_convert_int_status(int32_t int_status)
{
    if (int_status == 0) {
        return MPU6050_STATUS_OK;
    }

    if (int_status == 1) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    return MPU6050_STATUS_ERROR_COMM;
}

static int16_t mpu6050_merge_bytes(uint8_t high_byte, uint8_t low_byte)
{
    return (int16_t)(((uint16_t)high_byte << 8) | (uint16_t)low_byte);
}

static uint8_t mpu6050_build_gyro_config(mpu6050_gyro_fs_t gyro_fs)
{
    return (uint8_t)(((uint8_t)gyro_fs) << 3);
}

static uint8_t mpu6050_build_accel_config(mpu6050_accel_fs_t accel_fs)
{
    return (uint8_t)(((uint8_t)accel_fs) << 3);
}

static uint8_t mpu6050_build_dlpf_config(mpu6050_dlpf_cfg_t dlpf_cfg)
{
    return (uint8_t)(dlpf_cfg & 0x07U);
}

static bool mpu6050_is_valid_gyro_fs(mpu6050_gyro_fs_t gyro_fs)
{
    return ((uint8_t)gyro_fs <= (uint8_t)MPU6050_GYRO_FS_2000DPS);
}

static bool mpu6050_is_valid_accel_fs(mpu6050_accel_fs_t accel_fs)
{
    return ((uint8_t)accel_fs <= (uint8_t)MPU6050_ACCEL_FS_16G);
}

static bool mpu6050_is_valid_dlpf(mpu6050_dlpf_cfg_t dlpf_cfg)
{
    return ((uint8_t)dlpf_cfg <= (uint8_t)MPU6050_DLPF_5HZ);
}

static bool mpu6050_is_valid_init_config(const mpu6050_init_config_t *init_config)
{
    if (init_config == (const mpu6050_init_config_t *)0) {
        return false;
    }

    if (mpu6050_is_valid_gyro_fs(init_config->gyro_fs) == false) {
        return false;
    }

    if (mpu6050_is_valid_accel_fs(init_config->accel_fs) == false) {
        return false;
    }

    if (mpu6050_is_valid_dlpf(init_config->dlpf_cfg) == false) {
        return false;
    }

    return true;
}

static void mpu6050_store_current_config(mpu6050_t *me, const mpu6050_init_config_t *init_config)
{
    if (init_config == (const mpu6050_init_config_t *)0) {
        return;
    }

    me->current_config = *init_config;
    me->has_current_config = true;
}

mpu6050_status_t mpu6050_write_reg(mpu6050_t *me, uint8_t reg_addr, uint8_t reg_data)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if (me->cfg.i2c->write_reg == NULL) {
        return MPU6050_STATUS_ERROR_COMM;
    }
    return mpu6050_convert_int_status(me->cfg.i2c->write_reg(me->cfg.i2c_context, me->cfg.device_addr, reg_addr, reg_data));
}

mpu6050_status_t mpu6050_read_reg(mpu6050_t *me, uint8_t reg_addr, uint8_t *reg_data)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if (reg_data == (uint8_t *)0) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    if (me->cfg.i2c->read_reg == NULL) {
        return MPU6050_STATUS_ERROR_COMM;
    }
    return mpu6050_convert_int_status(me->cfg.i2c->read_reg(me->cfg.i2c_context, me->cfg.device_addr, reg_addr, reg_data));
}

mpu6050_status_t mpu6050_write_regs(mpu6050_t *me, uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if ((write_data == (const uint8_t *)0) || (write_length == 0U)) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    if (me->cfg.i2c->write_regs == NULL) {
        return MPU6050_STATUS_ERROR_COMM;
    }
    return mpu6050_convert_int_status(me->cfg.i2c->write_regs(me->cfg.i2c_context, me->cfg.device_addr, reg_addr, write_data, write_length));
}

mpu6050_status_t mpu6050_read_regs(mpu6050_t *me, uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if ((read_data == (uint8_t *)0) || (read_length == 0U)) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    if (me->cfg.i2c->read_regs == NULL) {
        return MPU6050_STATUS_ERROR_COMM;
    }
    return mpu6050_convert_int_status(me->cfg.i2c->read_regs(me->cfg.i2c_context, me->cfg.device_addr, reg_addr, read_data, read_length));
}

mpu6050_status_t mpu6050_get_who_am_i(mpu6050_t *me, uint8_t *who_am_i)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if (who_am_i == (uint8_t *)0) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    return mpu6050_read_reg(me, MPU6050_REG_WHO_AM_I, who_am_i);
}

mpu6050_status_t mpu6050_check_id(mpu6050_t *me)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_status_t driver_status;
    uint8_t who_am_i = 0U;

    if (me->cfg.i2c->check_device == NULL) {
        return MPU6050_STATUS_ERROR_COMM;
    }
    driver_status = mpu6050_convert_int_status(me->cfg.i2c->check_device(me->cfg.i2c_context, me->cfg.device_addr));
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    driver_status = mpu6050_get_who_am_i(me, &who_am_i);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        return MPU6050_STATUS_ERROR_ID;
    }

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_get_default_init_config(mpu6050_init_config_t *init_config)
{
    if (init_config == (mpu6050_init_config_t *)0) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    init_config->gyro_fs = MPU6050_GYRO_FS_2000DPS;
    init_config->accel_fs = MPU6050_ACCEL_FS_16G;
    init_config->dlpf_cfg = MPU6050_DLPF_5HZ;
    init_config->sample_rate_div = 9U;

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_get_current_config(mpu6050_t *me, mpu6050_init_config_t *init_config)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    if (init_config == (mpu6050_init_config_t *)0) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    if (me->has_current_config == false) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    *init_config = me->current_config;

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_init_with_config(mpu6050_t *me, const mpu6050_init_config_t *init_config)
{
    if (me == NULL) { return MPU6050_STATUS_ERROR_PARAM; }
    me->base.is_init = 0U;
    me->has_current_config = false;
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_status_t driver_status;
    uint8_t gyro_config;
    uint8_t accel_config;
    uint8_t dlpf_config;

    if (me->cfg.delay_ms == NULL || mpu6050_is_valid_init_config(init_config) == false) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    gyro_config = mpu6050_build_gyro_config(init_config->gyro_fs);
    accel_config = mpu6050_build_accel_config(init_config->accel_fs);
    dlpf_config = mpu6050_build_dlpf_config(init_config->dlpf_cfg);

    if (me->cfg.i2c->init != NULL) {
        driver_status = mpu6050_convert_int_status(me->cfg.i2c->init(me->cfg.i2c_context));
        if (driver_status != MPU6050_STATUS_OK) { return driver_status; }
    }

    driver_status = mpu6050_write_reg(me, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_MGMT_1_WAKEUP);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    me->cfg.delay_ms(me->cfg.delay_context, MPU6050_SENSOR_START_DELAY_MS);

    driver_status = mpu6050_check_id(me);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    driver_status = mpu6050_write_reg(me, MPU6050_REG_SMPLRT_DIV, init_config->sample_rate_div);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    driver_status = mpu6050_write_reg(me, MPU6050_REG_CONFIG, dlpf_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    driver_status = mpu6050_write_reg(me, MPU6050_REG_GYRO_CONFIG, gyro_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    driver_status = mpu6050_write_reg(me, MPU6050_REG_ACCEL_CONFIG, accel_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    mpu6050_store_current_config(me, init_config);
    me->base.is_init = 1U;

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_init(mpu6050_t *me)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_init_config_t init_config;
    mpu6050_status_t driver_status;

    driver_status = mpu6050_get_default_init_config(&init_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    return mpu6050_init_with_config(me, &init_config);
}

mpu6050_status_t mpu6050_init_default(mpu6050_t *me)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    return mpu6050_init(me);
}

mpu6050_status_t mpu6050_set_gyro_config(mpu6050_t *me, mpu6050_gyro_fs_t gyro_fs)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_init_config_t init_config;
    mpu6050_status_t driver_status;
    uint8_t gyro_config;

    if (mpu6050_is_valid_gyro_fs(gyro_fs) == false) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    driver_status = mpu6050_get_current_config(me, &init_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    gyro_config = mpu6050_build_gyro_config(gyro_fs);
    driver_status = mpu6050_write_reg(me, MPU6050_REG_GYRO_CONFIG, gyro_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    init_config.gyro_fs = gyro_fs;
    mpu6050_store_current_config(me, &init_config);

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_set_accel_config(mpu6050_t *me, mpu6050_accel_fs_t accel_fs)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_init_config_t init_config;
    mpu6050_status_t driver_status;
    uint8_t accel_config;

    if (mpu6050_is_valid_accel_fs(accel_fs) == false) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    driver_status = mpu6050_get_current_config(me, &init_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    accel_config = mpu6050_build_accel_config(accel_fs);
    driver_status = mpu6050_write_reg(me, MPU6050_REG_ACCEL_CONFIG, accel_config);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    init_config.accel_fs = accel_fs;
    mpu6050_store_current_config(me, &init_config);

    return MPU6050_STATUS_OK;
}

mpu6050_status_t mpu6050_read_raw_data(mpu6050_t *me, mpu6050_raw_data_t *raw_data)
{
    if (!mpu6050_is_bound(me)) { return MPU6050_STATUS_ERROR_PARAM; }
    mpu6050_status_t driver_status;
    uint8_t raw_buffer[MPU6050_RAW_DATA_LENGTH];

    if (me->base.is_init == 0U || raw_data == (mpu6050_raw_data_t *)0) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    driver_status = mpu6050_read_regs(me,
        MPU6050_REG_ACCEL_XOUT_H, raw_buffer, MPU6050_RAW_DATA_LENGTH);
    if (driver_status != MPU6050_STATUS_OK) {
        return driver_status;
    }

    raw_data->accel_x = mpu6050_merge_bytes(raw_buffer[0], raw_buffer[1]);
    raw_data->accel_y = mpu6050_merge_bytes(raw_buffer[2], raw_buffer[3]);
    raw_data->accel_z = mpu6050_merge_bytes(raw_buffer[4], raw_buffer[5]);
    raw_data->temperature = mpu6050_merge_bytes(raw_buffer[6], raw_buffer[7]);
    raw_data->gyro_x = mpu6050_merge_bytes(raw_buffer[8], raw_buffer[9]);
    raw_data->gyro_y = mpu6050_merge_bytes(raw_buffer[10], raw_buffer[11]);
    raw_data->gyro_z = mpu6050_merge_bytes(raw_buffer[12], raw_buffer[13]);

    return MPU6050_STATUS_OK;
}

const char *mpu6050_get_gyro_fs_string(mpu6050_gyro_fs_t gyro_fs)
{
    switch (gyro_fs) {
        case MPU6050_GYRO_FS_250DPS:
            return "+/-250dps";
        case MPU6050_GYRO_FS_500DPS:
            return "+/-500dps";
        case MPU6050_GYRO_FS_1000DPS:
            return "+/-1000dps";
        case MPU6050_GYRO_FS_2000DPS:
            return "+/-2000dps";
        default:
            return "UNKNOWN";
    }
}

const char *mpu6050_get_accel_fs_string(mpu6050_accel_fs_t accel_fs)
{
    switch (accel_fs) {
        case MPU6050_ACCEL_FS_2G:
            return "+/-2g";
        case MPU6050_ACCEL_FS_4G:
            return "+/-4g";
        case MPU6050_ACCEL_FS_8G:
            return "+/-8g";
        case MPU6050_ACCEL_FS_16G:
            return "+/-16g";
        default:
            return "UNKNOWN";
    }
}

const char *mpu6050_get_dlpf_string(mpu6050_dlpf_cfg_t dlpf_cfg)
{
    switch (dlpf_cfg) {
        case MPU6050_DLPF_260HZ:
            return "260Hz";
        case MPU6050_DLPF_184HZ:
            return "184Hz";
        case MPU6050_DLPF_94HZ:
            return "94Hz";
        case MPU6050_DLPF_44HZ:
            return "44Hz";
        case MPU6050_DLPF_21HZ:
            return "21Hz";
        case MPU6050_DLPF_10HZ:
            return "10Hz";
        case MPU6050_DLPF_5HZ:
            return "5Hz";
        default:
            return "UNKNOWN";
    }
}

float mpu6050_get_accel_lsb_per_g(mpu6050_accel_fs_t accel_fs)
{
    switch (accel_fs) {
        case MPU6050_ACCEL_FS_2G:
            return 16384.0f;
        case MPU6050_ACCEL_FS_4G:
            return 8192.0f;
        case MPU6050_ACCEL_FS_8G:
            return 4096.0f;
        case MPU6050_ACCEL_FS_16G:
            return 2048.0f;
        default:
            return 0.0f;
    }
}

float mpu6050_get_gyro_lsb_per_dps(mpu6050_gyro_fs_t gyro_fs)
{
    switch (gyro_fs) {
        case MPU6050_GYRO_FS_250DPS:
            return 131.0f;
        case MPU6050_GYRO_FS_500DPS:
            return 65.5f;
        case MPU6050_GYRO_FS_1000DPS:
            return 32.8f;
        case MPU6050_GYRO_FS_2000DPS:
            return 16.4f;
        default:
            return 0.0f;
    }
}

mpu6050_status_t mpu6050_convert_raw_data(
    const mpu6050_init_config_t *init_config,
    const mpu6050_raw_data_t *raw_data,
    mpu6050_scaled_data_t *scaled_data)
{
    float accel_lsb_per_g;
    float gyro_lsb_per_dps;

    if ((init_config == (const mpu6050_init_config_t *)0) ||
        (raw_data == (const mpu6050_raw_data_t *)0) ||
        (scaled_data == (mpu6050_scaled_data_t *)0)) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    accel_lsb_per_g = mpu6050_get_accel_lsb_per_g(init_config->accel_fs);
    gyro_lsb_per_dps = mpu6050_get_gyro_lsb_per_dps(init_config->gyro_fs);

    if ((accel_lsb_per_g <= 0.0f) || (gyro_lsb_per_dps <= 0.0f)) {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    scaled_data->temperature_c = ((float)raw_data->temperature / 340.0f) + 36.53f;
    scaled_data->accel_x_g = (float)raw_data->accel_x / accel_lsb_per_g;
    scaled_data->accel_y_g = (float)raw_data->accel_y / accel_lsb_per_g;
    scaled_data->accel_z_g = (float)raw_data->accel_z / accel_lsb_per_g;
    scaled_data->gyro_x_dps = (float)raw_data->gyro_x / gyro_lsb_per_dps;
    scaled_data->gyro_y_dps = (float)raw_data->gyro_y / gyro_lsb_per_dps;
    scaled_data->gyro_z_dps = (float)raw_data->gyro_z / gyro_lsb_per_dps;
    scaled_data->gyro_x_rad_s = scaled_data->gyro_x_dps * MPU6050_DEG_TO_RAD;
    scaled_data->gyro_y_rad_s = scaled_data->gyro_y_dps * MPU6050_DEG_TO_RAD;
    scaled_data->gyro_z_rad_s = scaled_data->gyro_z_dps * MPU6050_DEG_TO_RAD;

    return MPU6050_STATUS_OK;
}
