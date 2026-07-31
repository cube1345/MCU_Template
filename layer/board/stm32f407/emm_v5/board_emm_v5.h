/* SPDX-License-Identifier: MIT */
/**
 * @file    board_emm_v5.h
 * @brief   STM32F407板级EMM V5闭环步进电机访问接口。
 * @author  weixin
 */
#ifndef BOARD_EMM_V5_H
#define BOARD_EMM_V5_H

#include "device/motor/motor.h"

/* WEIXIN允许设置的EMM绝对角度范围，单位为度。 */
#define BOARD_EMM_V5_POSITION_DEG_MIN (-45.0F)
#define BOARD_EMM_V5_POSITION_DEG_MAX (45.0F)
/* EMM编码器原始位置允许的绝对值上限。 */
#define BOARD_EMM_V5_POSITION_RAW_ABS_LIMIT INT32_C(7000)

/**
 * @brief  绑定板载EMM V5电机对象和其USART6通信资源。
 * @retval PLATFORM_OK表示绑定成功或已经绑定，其他PLATFORM_*表示失败。
 * @note   电机使用USART6的PC6(TX)/PC7(RX)。本函数启动UART但不发送运动命令。
 */
int board_emm_v5_init(void);

/**
 * @brief  获取板载EMM V5对应的motor基础对象。
 * @retval board_emm_v5_init()成功后返回motor对象，否则返回NULL。
 */
struct motor *board_emm_v5_get(void);

/**
 * @brief  通过USART6读取板载EMM V5的实时位置。
 * @param  position_raw 用于接收实时位置原始值的变量。
 * @retval PLATFORM_OK表示读取成功，其他PLATFORM_*表示状态或通信错误。
 * @note   原始位置每65536单位表示电机旋转一圈，仅由EMM任务调用。
 */
int board_emm_v5_position_read(int32_t *position_raw);

/**
 * @brief  查询最近一次位置采样是否触发软件限位。
 * @retval true表示位于正向或负向限位外，false表示位于安全范围内。
 */
bool board_emm_v5_position_is_limited(void);

/**
 * @brief  立即停止当前运动，但保持EMM使能和motor对象启动状态。
 * @retval PLATFORM_OK表示停止命令发送成功，其他值表示状态或通信错误。
 * @note   用于软件限位，停止后仍可发送返回安全区的运动命令。
 */
int board_emm_v5_motion_stop(void);

/**
 * @brief  将EMM当前位置设置为0度。
 * @retval PLATFORM_OK表示清零成功，其他值表示状态或通信错误。
 * @note   只重定义软件坐标，不执行机械回零。
 */
int board_emm_v5_position_clear(void);

/**
 * @brief  驱动EMM移动到指定绝对角度。
 * @param  position_deg 相对启动零点的目标角度，单位为度。
 * @retval PLATFORM_OK表示命令发送成功，其他值表示参数或通信错误。
 * @note   当前配置按1.8度电机、16细分换算为3200脉冲每圈。
 */
int board_emm_v5_position_deg_set(float position_deg);

#endif /* BOARD_EMM_V5_H */
