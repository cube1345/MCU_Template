/* SPDX-License-Identifier: MIT */
/**
 * @file    emm_v5_soft_limit.h
 * @brief   EMM V5速度与位置命令软件限位修饰器。
 * @author  weixin
 *
 * 本组件不分配堆内存，运行期间独占inner对象。所有访问均为阻塞式任务上下文
 * 调用，调用方必须在同一控制任务内串行执行，禁止在中断或多个任务中并发调用。
 */
#ifndef EMM_V5_SOFT_LIMIT_H
#define EMM_V5_SOFT_LIMIT_H

#include "emm_v5/emm_v5.h"

#include <stdbool.h>
#include <stdint.h>

enum emm_v5_soft_limit_state {
  /* 编码器位置位于正负软件限位之间。 */
  EMM_V5_SOFT_LIMIT_NONE = 0,
  /* 编码器位置超过正向软件限位。 */
  EMM_V5_SOFT_LIMIT_POSITIVE,
  /* 编码器位置超过负向软件限位。 */
  EMM_V5_SOFT_LIMIT_NEGATIVE,
};

struct emm_v5_soft_limit_config {
  /* 编码器原始位置允许的绝对值上限。 */
  int32_t position_raw_abs_limit;
  /* EMM编码器旋转一圈对应的原始位置计数。 */
  uint32_t encoder_counts_per_revolution;
  /* EMM位置命令旋转一圈对应的脉冲数。 */
  uint32_t command_pulses_per_revolution;
};

struct emm_v5_soft_limit {
  /* 向上层暴露的motor对象，必须保持为首成员。 */
  struct motor base;
  /* 被修饰的原始EMM V5对象，由本组件独占其控制生命周期。 */
  struct emm_v5 *inner;
  /* 软件限位和两种位置单位的换算配置。 */
  struct emm_v5_soft_limit_config config;
  /* 最近一次成功读取的编码器原始位置。 */
  int32_t position_raw;
  /* 根据最近位置样本计算出的限位方向。 */
  enum emm_v5_soft_limit_state limit_state;
  /* 首次位置读取成功后置true，未建立位置基准时拒绝运动命令。 */
  bool position_valid;
};

/**
 * @brief  初始化EMM V5软件限位修饰器并绑定motor基础对象。
 * @param  self 调用者提供的修饰器对象。
 * @param  name 修饰后motor对象的逻辑名称，生命周期必须长于self。
 * @param  inner 被修饰的原始EMM V5对象，运行期间由self独占。
 * @param  config 限位阈值和位置单位换算配置。
 * @retval PLATFORM_OK表示初始化成功，其他值表示参数、配置或状态错误。
 */
int emm_v5_soft_limit_init(
    struct emm_v5_soft_limit *self,
    const char *name,
    struct emm_v5 *inner,
    const struct emm_v5_soft_limit_config *config);

/**
 * @brief  获取修饰后的motor基础对象。
 * @param  self 已初始化的软件限位修饰器。
 * @retval self有效时返回motor对象，否则返回NULL。
 */
struct motor *emm_v5_soft_limit_get_motor(
    struct emm_v5_soft_limit *self);

/**
 * @brief  读取实时位置、更新限位状态并在首次越界时停止当前运动。
 * @param  self 已启动的软件限位修饰器。
 * @param  position_raw 用于接收编码器原始位置的变量。
 * @retval PLATFORM_OK表示读取和限位处理成功，其他值表示状态或通信错误。
 */
int emm_v5_soft_limit_position_read(
    struct emm_v5_soft_limit *self,
    int32_t *position_raw);

/**
 * @brief  将当前位置设置为零点并同步修饰器的位置状态。
 * @param  self 已启动且当前未越界的软件限位修饰器。
 * @retval PLATFORM_OK表示清零成功，PLATFORM_ERR_BUSY表示越界时禁止清零。
 */
int emm_v5_soft_limit_position_clear(
    struct emm_v5_soft_limit *self);

/**
 * @brief  发送经过软件限位检查的绝对位置命令。
 * @param  self 已启动的软件限位修饰器。
 * @param  pulse 相对驱动器零点的绝对位置脉冲数。
 * @retval PLATFORM_OK表示命令已发送，PLATFORM_ERR_BUSY表示命令会扩大越界。
 */
int emm_v5_soft_limit_move_absolute(
    struct emm_v5_soft_limit *self,
    int32_t pulse);

/**
 * @brief  立即停止当前运动并保持EMM使能和修饰器启动状态。
 * @param  self 已启动的软件限位修饰器。
 * @retval PLATFORM_OK表示停止命令已发送，其他值表示状态或通信错误。
 */
int emm_v5_soft_limit_motion_stop(
    struct emm_v5_soft_limit *self);

/**
 * @brief  获取最近位置样本对应的软件限位状态。
 * @param  self 已初始化的软件限位修饰器。
 * @retval 当前限位方向；对象或位置样本无效时返回EMM_V5_SOFT_LIMIT_NONE。
 */
enum emm_v5_soft_limit_state emm_v5_soft_limit_state_get(
    const struct emm_v5_soft_limit *self);

#endif /* EMM_V5_SOFT_LIMIT_H */
