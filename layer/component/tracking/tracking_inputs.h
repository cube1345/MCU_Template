/* SPDX-License-Identifier: MIT */
/**
 * @file    tracking_inputs.h
 * @brief   八路数字循迹输入采样组件。
 * @author  weixin
 */
#ifndef TRACKING_INPUTS_H
#define TRACKING_INPUTS_H

#include <stdbool.h>
#include <stdint.h>

/* 循迹组件固定管理的数字输入数量。 */
#define TRACKING_INPUTS_COUNT UINT8_C(8)

/** @brief 八路循迹输入的静态引脚配置。 */
struct tracking_inputs_config {
  int32_t pins[TRACKING_INPUTS_COUNT];
};

/** @brief 八路循迹输入组件对象，由调用者持有其存储空间。 */
struct tracking_inputs {
  struct tracking_inputs_config config;
  bool initialized;
};

/** @brief 一次顺序采样得到的八路原始输入电平。 */
struct tracking_inputs_state {
  bool high[TRACKING_INPUTS_COUNT];
  uint8_t high_mask;
};

/**
 * @brief  初始化八路循迹输入组件并配置所有 GPIO。
 * @param  self 调用者持有的组件对象。
 * @param  config 生命周期仅需覆盖本次调用的八路引脚配置。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   调用前必须先注册 Platform Pin 后端；GPIO 配置为内部上拉输入。
 */
int tracking_inputs_init(
    struct tracking_inputs *self,
    const struct tracking_inputs_config *config);

/**
 * @brief  读取一个循迹输入通道的原始电平。
 * @param  self 已初始化的组件对象。
 * @param  channel 通道编号，范围为 0 至 TRACKING_INPUTS_COUNT - 1。
 * @param  high 用于接收原始电平，true 表示高电平。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   本函数不修改组件状态，不执行滤波或有效电平反转。
 */
int tracking_inputs_read_channel(
    const struct tracking_inputs *self,
    uint8_t channel,
    bool *high);

/**
 * @brief  顺序读取全部八路循迹输入。
 * @param  self 已初始化的组件对象。
 * @param  state 用于接收电平数组和高电平位掩码。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   high_mask 的 bit0 至 bit7 对应通道 0 至 7；采样不是硬件同时锁存。
 */
int tracking_inputs_sample(
    const struct tracking_inputs *self,
    struct tracking_inputs_state *state);

#endif /* TRACKING_INPUTS_H */
