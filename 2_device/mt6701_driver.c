/**
 * @file    mt6701_driver.c
 * @brief   MT6701 14-bit 磁编码器 GPIO SSI 驱动实现 (双编码器)
 * @author  cube
 * @date    2026-05-02
 *
 * 共享 SCLK/DO 总线, 通过各自 CSN 片选分时读取。
 *
 * SSI 协议帧 (24-bit, MSB first):
 *   [23:10] angle[13:0]  14-bit 绝对角度
 *   [ 9: 6] status[3:0]   4-bit 磁场状态
 *   [ 5: 0] crc[5:0]      6-bit CRC (多项式 0x43)
 *
 * 分层说明:
 * - 本文件实现 SSI 协议逻辑、CRC 校验、角度换算、速度计算
 * - 不包含任何具体引脚号, 引脚信息由 Board 层通过 MT6701_Init() 注入
 * - GPIO 读写通过 HAL 层 hal_gpio.h (HAL_GPIO_* 接口)
 */

#include "mt6701_driver.h"

#include <math.h>
#include <stddef.h>

#include "hal_gpio.h"

/* CRC-6 多项式: x^6 + x + 1 → 0x43 */
#define MT6701_CRC_POLY          0x43U
#define MT6701_CRC_WIDTH         6U

/* ------------------------------------------------------------------ */
/* Board 层注入的 SSI 总线引脚配置                                      */
/* ------------------------------------------------------------------ */

static mt6701_bus_cfg_t g_bus_cfg;

/* ------------------------------------------------------------------ */
/* Per-axis 运行时状态                                                 */
/* ------------------------------------------------------------------ */

static int32_t  g_last_angle_raw[MT6701_AXIS_COUNT];
static uint8_t  g_last_field_status[MT6701_AXIS_COUNT];
static bool     g_last_crc_valid[MT6701_AXIS_COUNT];
static float    g_motor_speed[MT6701_AXIS_COUNT];
static float    g_last_angle_rad[MT6701_AXIS_COUNT];
static bool     g_speed_calc_ready[MT6701_AXIS_COUNT];

/* ------------------------------------------------------------------ */
/* 内部函数                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief CRC-6 校验
 * @param data 待校验的 18-bit 数据 (angle[13:0] + status[3:0])
 * @return 6-bit CRC 值
 */
static uint8_t MT6701_CalculateCRC(uint32_t data)
{
    uint8_t crc = 0;

    for (int i = 17; i >= 0; i--) {
        uint8_t bit = (data >> i) & 1U;
        crc <<= 1;
        if ((crc >> MT6701_CRC_WIDTH) ^ bit) {
            crc ^= MT6701_CRC_POLY;
        }
        crc &= (uint8_t)((1U << MT6701_CRC_WIDTH) - 1U);
    }
    return crc;
}

/**
 * @brief GPIO 快速软件翻转 — 产生一个 SCK 脉冲并读取 DO
 * @param bitPos 当前 bit 在帧中的位置 (MSB first)
 * @param pFrame 帧数据累积指针
 *
 * 使用 Board 层注入的 g_bus_cfg 中存储的引脚信息,
 * 通过 HAL 层 HAL_GPIO_* 操作 GPIO。
 * 声明为 __STATIC_INLINE (static inline) 以消除 24 次循环调用的函数开销。
 */
__STATIC_INLINE void MT6701_ClockBit(uint32_t bitPos, uint32_t *pFrame)
{
    HAL_GPIO_SetPin(g_bus_cfg.sclk.port, g_bus_cfg.sclk.pin);

    if (HAL_GPIO_ReadPin(g_bus_cfg.dio.port, g_bus_cfg.dio.pin)) {
        *pFrame |= (1UL << bitPos);
    }

    HAL_GPIO_ClearPin(g_bus_cfg.sclk.port, g_bus_cfg.sclk.pin);
}

/**
 * @brief 对单轴执行一次原子 SSI 读取, 解析并存储结果
 * @param axis 轴编号
 * @return 0~16383 有效角度, -1 表示 CRC 失败
 */
static int32_t MT6701_ReadAxisRawAtomic(MT6701_Axis_t axis)
{
    const mt6701_gpio_t *csn = &g_bus_cfg.csn[axis];
    uint32_t frame = 0U;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* 拉低目标 CSN → 启动 SSI 帧 */
    HAL_GPIO_ClearPin(csn->port, csn->pin);

    /* 24 个 SCK 脉冲, MSB first */
    for (uint32_t i = MT6701_SSI_FRAME_BITS; i > 0U; i--) {
        MT6701_ClockBit(i - 1U, &frame);
    }

    /* 拉高 CSN → 结束帧 */
    HAL_GPIO_SetPin(csn->port, csn->pin);

    if (!primask) {
        __enable_irq();
    }

    /* 解析帧: [23:10] angle, [9:6] status, [5:0] crc */
    uint32_t angle_raw  = (frame >> 10) & 0x3FFFU;
    uint32_t field_stat = (frame >> 6)  & 0x0FU;
    uint32_t crc_rx     = frame & 0x3FU;

    uint32_t crc_data   = angle_raw << 4 | field_stat;
    uint8_t  crc_calc   = MT6701_CalculateCRC(crc_data);

    g_last_field_status[axis] = (uint8_t)field_stat;
    g_last_crc_valid[axis]    = (crc_calc == (uint8_t)crc_rx);

    if (g_last_crc_valid[axis]) {
        g_last_angle_raw[axis] = (int32_t)angle_raw;
        return g_last_angle_raw[axis];
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* 公开接口                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief 初始化 MT6701 内部状态并绑定 SSI 总线引脚
 * @param bus_cfg SSI 总线引脚配置, 由 Board 层提供
 *
 * Board 层调用顺序:
 * 1. HAL_GPIO_InitOutput / HAL_GPIO_InitInput 初始化 GPIO IOMUX
 * 2. 填充 mt6701_bus_cfg_t 并调用本函数
 */
void MT6701_Init(const mt6701_bus_cfg_t *bus_cfg)
{
    if (bus_cfg != ((const mt6701_bus_cfg_t *)0)) {
        g_bus_cfg = *bus_cfg;
    }

    for (uint32_t a = 0U; a < MT6701_AXIS_COUNT; a++) {
        g_last_angle_raw[a]    = -1;
        g_last_field_status[a] = 0U;
        g_last_crc_valid[a]    = false;
        g_motor_speed[a]       = 0.0f;
        g_last_angle_rad[a]    = 0.0f;
        g_speed_calc_ready[a]  = false;
    }
}

int32_t MT6701_ReadAngleRaw(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return -1;
    }
    return MT6701_ReadAxisRawAtomic(axis);
}

float MT6701_ReadAngleRad(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return 0.0f;
    }
    int32_t raw = MT6701_ReadAxisRawAtomic(axis);
    if (raw < 0) {
        return g_last_angle_rad[axis];
    }
    g_last_angle_rad[axis] = (float)raw * (float)(2.0 * 3.14159265358979323846)
                             / (float)(MT6701_ANGLE_MAX);
    return g_last_angle_rad[axis];
}

float MT6701_ReadAngleDeg(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return 0.0f;
    }
    int32_t raw = MT6701_ReadAxisRawAtomic(axis);
    if (raw < 0) {
        return g_last_angle_rad[axis] * (float)(180.0 / 3.14159265358979323846);
    }
    return (float)raw * 360.0f / (float)(MT6701_ANGLE_MAX);
}

uint8_t MT6701_GetFieldStatus(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return 0U;
    }
    return g_last_field_status[axis];
}

int32_t MT6701_GetLastAngleRaw(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return -1;
    }
    return g_last_angle_raw[axis];
}

bool MT6701_IsLastCRCValid(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return false;
    }
    return g_last_crc_valid[axis];
}

void MT6701_Service1ms(void)
{
    const float pi2 = (float)(2.0 * 3.14159265358979323846);

    for (uint32_t a = 0U; a < MT6701_AXIS_COUNT; a++) {
        float angle_rad = MT6701_ReadAngleRad((MT6701_Axis_t)a);

        if (!g_speed_calc_ready[a]) {
            g_last_angle_rad[a]   = angle_rad;
            g_speed_calc_ready[a] = true;
            continue;
        }

        float diff = angle_rad - g_last_angle_rad[a];

        if (diff > (float)(3.14159265358979323846)) {
            diff -= pi2;
        } else if (diff < (float)(-3.14159265358979323846)) {
            diff += pi2;
        }

        g_motor_speed[a]    = diff * 1000.0f;
        g_last_angle_rad[a] = angle_rad;
    }
}

float MT6701_GetSpeed(MT6701_Axis_t axis)
{
    if (axis >= MT6701_AXIS_COUNT) {
        return 0.0f;
    }
    return g_motor_speed[axis];
}
