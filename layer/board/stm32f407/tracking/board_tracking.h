/* SPDX-License-Identifier: MIT */
/**
 * @file    board_tracking.h
 * @brief   STM32F407 八路循迹模块数字输入接口。
 * @author  weixin
 */
#ifndef BOARD_TRACKING_H
#define BOARD_TRACKING_H

#include "tracking_inputs.h"

#include <stdbool.h>
#include <stdint.h>

/* Board 层公开的循迹输入数量与组件固定通道数一致。 */
#define BOARD_TRACKING_INPUT_COUNT TRACKING_INPUTS_COUNT

/**
 * @brief  初始化右排针 15 至 8 号位的八路循迹输入。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU GPIO 后端初始化失败。
 * @note   所有输入均配置为带内部上拉的 GPIO 输入。
 */
int board_tracking_init(void);

/**
 * @brief  读取指定循迹输入通道的原始电平。
 * @param  channel 通道编号，范围为 0 至 BOARD_TRACKING_INPUT_COUNT - 1。
 * @param  high 用于接收原始电平，true 表示高电平。
 * @return PLATFORM_OK 表示读取成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   通道 0 至 7 依次对应右排针 15 至 8 号位，不做有效电平反转。
 */
int board_tracking_read(uint8_t channel, bool *high);

/**
 * @brief  获取已初始化的八路循迹输入组件。
 * @return 初始化成功后返回组件对象，否则返回 NULL。
 * @note   返回对象由 Board 层持有，调用者不得修改或释放。
 */
struct tracking_inputs *board_tracking_get(void);

#endif /* BOARD_TRACKING_H */
