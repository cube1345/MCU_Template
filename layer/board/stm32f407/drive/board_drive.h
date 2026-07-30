/* SPDX-License-Identifier: MIT */
/**
 * @file    board_drive.h
 * @brief   STM32F407双路AT8236闭环电机轴板级组合。
 * @author  weixin
 */
#ifndef BOARD_DRIVE_H
#define BOARD_DRIVE_H

#include "device/encoder/encoder.h"
#include "device/motor/motor.h"

/* 左右闭环电机轴的固定更新周期，单位为毫秒。 */
#define BOARD_DRIVE_UPDATE_PERIOD_MS UINT32_C(5)
/* 左右速度环默认比例增益。 */
#define BOARD_DRIVE_SPEED_KP_DEFAULT (0.044f)
/* 左右速度环默认积分增益。 */
#define BOARD_DRIVE_SPEED_KI_DEFAULT (0.015f)

/**
 * @brief  创建并绑定双路电机、编码器和闭环电机轴对象。
 * @return PLATFORM_OK 表示成功或已经绑定，其他 PLATFORM_* 表示失败。
 * @note   本函数不会启动 PWM 或编码器计数。绑定成功后可通过 getter
 *         获取闭环电机轴的 motor 基础对象；底层电机和编码器由轴独占。
 */
int board_drive_bind(void);

/**
 * @brief  启动双路闭环电机轴、PWM 和硬件编码器。
 * @return PLATFORM_OK 表示成功或已经启动，其他 PLATFORM_* 表示失败。
 * @note   初始化后两台电机均处于滑行状态，两个编码器从零开始累计。
 */
int board_drive_init(void);

/**
 * @brief  执行一次左右电机轴的编码器采样和速度闭环计算。
 * @return PLATFORM_OK 表示两侧更新成功，其他 PLATFORM_* 表示失败。
 * @note   必须从单一任务以 5 ms 固定周期调用，不能在中断中调用。
 */
int board_drive_update(void);

/**
 * @brief  同时更新左右闭环电机轴的速度 PI 参数。
 * @param  speed_kp 比例增益，范围由 motor_axis 组件约束。
 * @param  speed_ki 积分增益，范围由 motor_axis 组件约束。
 * @return PLATFORM_OK 表示两侧参数均已应用，其他 PLATFORM_* 表示失败。
 * @note   只能由闭环控制任务在控制周期边界调用。
 */
int board_drive_set_speed_gains(float speed_kp, float speed_ki);

/**
 * @brief  获取左侧闭环电机轴的 motor 基础对象。
 * @return board_drive_bind() 成功后返回 Motor 对象，否则返回 NULL。
 * @note   返回对象可能尚未启动，可通过 motor_is_initialized() 查询。
 */
struct motor *board_motor_left_get(void);

/**
 * @brief  获取右侧闭环电机轴的 motor 基础对象。
 * @return board_drive_bind() 成功后返回 Motor 对象，否则返回 NULL。
 * @note   返回对象可能尚未启动，可通过 motor_is_initialized() 查询。
 */
struct motor *board_motor_right_get(void);

/**
 * @brief  获取左侧编码器基础对象。
 * @return board_drive_bind() 成功后返回 Encoder 对象，否则返回 NULL。
 * @note   编码器由闭环电机轴独占，仅用于诊断；轴运行时禁止直接读取。
 */
struct encoder *board_encoder_left_get(void);

/**
 * @brief  获取右侧编码器基础对象。
 * @return board_drive_bind() 成功后返回 Encoder 对象，否则返回 NULL。
 * @note   编码器由闭环电机轴独占，仅用于诊断；轴运行时禁止直接读取。
 */
struct encoder *board_encoder_right_get(void);

#endif /* BOARD_DRIVE_H */
