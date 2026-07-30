/* SPDX-License-Identifier: MIT */
/**
 * @file    emm_v5.h
 * @brief   EMM V5闭环步进电机串口驱动。
 * @author  weixin
 *
 * 本驱动将EMM V5的串口控制协议适配为motor基础对象的速度和位置能力。
 * 所有命令均使用调用方绑定的抽象UART阻塞发送，驱动不依赖HAL类型。
 */
#ifndef EMM_V5_H
#define EMM_V5_H

#include "device/motor/motor.h"
#include "platform/inferface/uart/uart.h"

#include <stdbool.h>
#include <stdint.h>

/* EMM V5速度模式允许的绝对转速上限，单位为RPM。 */
#define EMM_V5_SPEED_RPM_MAX INT32_C(5000)

struct emm_v5_config {
  /* 电机地址；1通常为出厂默认地址，0为广播地址。 */
  uint8_t address;
  /* 标准位置模式使用的运行速度，单位为RPM，范围1至5000。 */
  uint16_t position_speed_rpm;
  /* 速度与位置命令的加速度，0表示直接启动。 */
  uint8_t acceleration;
  /* 单帧串口发送超时，单位为ms。 */
  uint32_t timeout_ms;
  /* true表示每条控制命令发送后必须收到四字节成功应答。 */
  bool response_required;
  /* true表示反转应用层定义的正方向。 */
  bool reversed;
};

struct emm_v5 {
  /* 必须作为首成员，供应用层向上转换为struct motor。 */
  struct motor base;
  /* 由板级层创建并长期持有的UART抽象对象。 */
  struct uart *uart;
  /* 板级固定的协议配置副本。 */
  struct emm_v5_config config;
  /* 最近一次成功下发的目标速度，单位为RPM。 */
  int32_t commanded_speed_rpm;
  /* 最近一次成功下发的标准位置相对脉冲数。 */
  int32_t commanded_position_pulse;
};

/**
 * @brief  绑定EMM V5具体对象与motor基础能力表。
 * @param  self 调用方持有的EMM V5对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  uart 已由Board层创建并启动的阻塞接收UART对象。
 * @param  config 电机地址、位置速度、加速度和超时配置。
 * @retval PLATFORM_OK表示绑定成功，其他PLATFORM_*表示参数或状态错误。
 * @note   本函数只建立软件对象关系，不会发送协议帧或访问硬件。
 */
int emm_v5_init(struct emm_v5 *self,
                const char *name,
                struct uart *uart,
                const struct emm_v5_config *config);

/**
 * @brief  获取EMM V5对应的motor基础对象。
 * @param  self EMM V5具体对象。
 * @retval self有效时返回其motor基础对象，否则返回NULL。
 */
struct motor *emm_v5_get_motor(struct emm_v5 *self);

/**
 * @brief  设置EMM V5速度模式的目标转速。
 * @param  self 已启动的EMM V5对象。
 * @param  speed_rpm 范围-5000至5000，符号表示方向，单位为RPM。
 * @retval PLATFORM_OK表示命令发送成功，其他PLATFORM_*表示状态或通信错误。
 * @note   speed_rpm为0时仍发送速度模式停止命令；紧急停止应使用emm_v5_stop_now()。
 */
int emm_v5_set_speed_rpm(struct emm_v5 *self, int32_t speed_rpm);

/**
 * @brief  以标准位置模式移动指定的相对脉冲数。
 * @param  self 已启动的EMM V5对象。
 * @param  pulse 有符号相对脉冲数，正负方向受reversed配置影响。
 * @retval PLATFORM_OK表示命令发送成功，其他PLATFORM_*表示状态或通信错误。
 * @note   位置模式参数由初始化配置中的position_speed_rpm和acceleration决定。
 */
int emm_v5_move_relative(struct emm_v5 *self, int32_t pulse);

/**
 * @brief  立即停止EMM V5当前运动。
 * @param  self 已启动的EMM V5对象。
 * @retval PLATFORM_OK表示停止命令发送成功，其他PLATFORM_*表示状态或通信错误。
 * @note   本函数只发送协议停止帧；UART生命周期由Board层统一维护。
 */
int emm_v5_stop_now(struct emm_v5 *self);

#endif /* EMM_V5_H */
