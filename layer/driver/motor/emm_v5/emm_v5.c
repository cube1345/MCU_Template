/* SPDX-License-Identifier: MIT */
/**
 * @file    emm_v5.c
 * @brief   EMM V5闭环步进电机串口协议实现。
 * @author  weixin
 */
#include "emm_v5.h"

#include "container_of.h"
#include "platform/inferface/uart/uart_blocking.h"
#include "result.h"
#include "timebase.h"

#include <limits.h>
#include <stddef.h>

/* EMM V5帧尾固定校验字节。 */
#define EMM_V5_FRAME_CHECKSUM UINT8_C(0x6B)
/* EMM V5使能控制功能码和辅助字节。 */
#define EMM_V5_COMMAND_ENABLE UINT8_C(0xF3)
#define EMM_V5_ENABLE_MAGIC UINT8_C(0xAB)
/* EMM V5速度、标准位置和立即停止功能码。 */
#define EMM_V5_COMMAND_SPEED UINT8_C(0xF6)
#define EMM_V5_COMMAND_POSITION UINT8_C(0xFD)
#define EMM_V5_COMMAND_STOP_NOW UINT8_C(0xFE)
#define EMM_V5_COMMAND_READ_POSITION UINT8_C(0x36)
#define EMM_V5_COMMAND_CLEAR_POSITION UINT8_C(0x0A)
#define EMM_V5_STOP_MAGIC UINT8_C(0x98)
#define EMM_V5_CLEAR_POSITION_MAGIC UINT8_C(0x6D)
/* EMM V5协议方向值：0为顺时针，1为逆时针。 */
#define EMM_V5_DIRECTION_CW UINT8_C(0)
#define EMM_V5_DIRECTION_CCW UINT8_C(1)
/* 所有命令均立即生效，不参与多机同步缓存。 */
#define EMM_V5_SYNC_DISABLED UINT8_C(0)
/* 标准位置模式使用相对当前实时位置的运动语义。 */
#define EMM_V5_POSITION_RELATIVE UINT8_C(0)
#define EMM_V5_POSITION_ABSOLUTE UINT8_C(1)
/* 控制动作命令固定返回四字节应答，状态0x02表示执行成功。 */
#define EMM_V5_RESPONSE_SIZE UINT16_C(4)
#define EMM_V5_RESPONSE_SUCCESS UINT8_C(0x02)
#define EMM_V5_RESPONSE_CONDITION_FAILED UINT8_C(0xE2)
/* 实时位置应答包含地址、功能码、符号、四字节位置和校验字节。 */
#define EMM_V5_POSITION_RESPONSE_SIZE UINT16_C(8)
#define EMM_V5_POSITION_SIGN_POSITIVE UINT8_C(0)
#define EMM_V5_POSITION_SIGN_NEGATIVE UINT8_C(1)
/* 每条EMM V5命令发送前的固定等待时间，单位为ms。 */
#define EMM_V5_COMMAND_PRE_DELAY_MS UINT32_C(3)

/**
 * @brief  检查EMM V5是否已经完成motor生命周期启动。
 * @param  self EMM V5具体对象。
 * @retval PLATFORM_OK表示可发送命令，其他PLATFORM_*表示对象无效或未启动。
 */
static int emm_v5_ready_check(const struct emm_v5 *self)
{
  if ((self == NULL) || (self->uart == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->base.initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return PLATFORM_OK;
}

/**
 * @brief  通过已绑定UART发送一帧EMM V5协议数据。
 * @param  self EMM V5具体对象。
 * @param  frame 调用期间保持有效的协议帧缓冲区。
 * @param  size 协议帧有效字节数。
 * @retval PLATFORM_OK表示发送成功，其他PLATFORM_*表示状态或串口错误。
 */
static int emm_v5_exchange(const struct emm_v5 *self,
                           const uint8_t *frame,
                           uint16_t size)
{
  uint8_t response[EMM_V5_RESPONSE_SIZE];
  int result;

  if ((self == NULL) || (self->uart == NULL) ||
      (frame == NULL) || (size < UINT16_C(2))) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  /* 统一预留驱动器命令处理间隔，避免连续控制帧贴得过近。 */
  delay_ms(EMM_V5_COMMAND_PRE_DELAY_MS);
  result = uart_write(self->uart, frame, size, self->config.timeout_ms);
  if ((result != PLATFORM_OK) || !self->config.response_required) {
    /* 无应答模式与官方上位机一致，HAL发送完成即认为命令已经下发。 */
    return result;
  }
  if (result == PLATFORM_OK) {
    result = uart_blocking_read(self->uart, response,
                                EMM_V5_RESPONSE_SIZE,
                                self->config.timeout_ms);
  }
  if (result != PLATFORM_OK) {
    return result;
  }

  /* 正常应答必须完整回显设备地址、功能码、成功状态和校验字节。 */
  if ((response[0] == self->config.address) &&
      (response[1] == frame[1]) &&
      (response[2] == EMM_V5_RESPONSE_SUCCESS) &&
      (response[3] == EMM_V5_FRAME_CHECKSUM)) {
    return PLATFORM_OK;
  }
  if ((response[0] == self->config.address) &&
      (response[1] == frame[1]) &&
      (response[2] == EMM_V5_RESPONSE_CONDITION_FAILED) &&
      (response[3] == EMM_V5_FRAME_CHECKSUM)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return PLATFORM_ERR_IO;
}

/**
 * @brief  根据有符号速度计算EMM V5协议方向与无符号速度值。
 * @param  self EMM V5具体对象。
 * @param  speed_rpm 范围-5000至5000的目标速度。
 * @param  direction 输出协议方向字节。
 * @param  magnitude_rpm 输出非负速度幅值。
 * @retval 无返回值。
 */
static void emm_v5_speed_encode(const struct emm_v5 *self,
                                int32_t speed_rpm,
                                uint8_t *direction,
                                uint16_t *magnitude_rpm)
{
  bool ccw = speed_rpm < 0;
  int32_t magnitude = ccw ? -speed_rpm : speed_rpm;

  if (self->config.reversed) {
    ccw = !ccw;
  }
  *direction = ccw ? EMM_V5_DIRECTION_CCW : EMM_V5_DIRECTION_CW;
  *magnitude_rpm = (uint16_t)magnitude;
}

/**
 * @brief  通过已经启动的板级串口使能电机。
 * @param  base motor基础对象。
 * @retval PLATFORM_OK表示启动成功，其他PLATFORM_*表示通信或资源错误。
 */
static int emm_v5_start(struct motor *base)
{
  struct emm_v5 *self = container_of(base, struct emm_v5, base);
  uint8_t enable_frame[6];
  int result;

  enable_frame[0] = self->config.address;
  enable_frame[1] = EMM_V5_COMMAND_ENABLE;
  enable_frame[2] = EMM_V5_ENABLE_MAGIC;
  enable_frame[3] = UINT8_C(1);
  enable_frame[4] = EMM_V5_SYNC_DISABLED;
  enable_frame[5] = EMM_V5_FRAME_CHECKSUM;
  result = emm_v5_exchange(self, enable_frame, sizeof(enable_frame));
  return result;
}

/**
 * @brief  发送立即停止帧，板级UART资源保持运行。
 * @param  base motor基础对象。
 * @retval PLATFORM_OK表示停止和资源释放成功，其他PLATFORM_*表示首个错误。
 */
static int emm_v5_stop(struct motor *base)
{
  struct emm_v5 *self = container_of(base, struct emm_v5, base);
  return emm_v5_stop_now(self);
}

/**
 * @brief  通过motor速度能力设置EMM V5目标转速。
 * @param  base motor基础对象。
 * @param  speed_rpm 范围-5000至5000的目标速度。
 * @retval PLATFORM_OK表示发送成功，其他PLATFORM_*表示参数或通信错误。
 */
static int emm_v5_set_speed(struct motor *base, int32_t speed_rpm)
{
  return emm_v5_set_speed_rpm(
      container_of(base, struct emm_v5, base), speed_rpm);
}

/**
 * @brief  通过motor位置能力执行EMM V5相对脉冲移动。
 * @param  base motor基础对象。
 * @param  pulse 有符号相对脉冲数。
 * @retval PLATFORM_OK表示发送成功，其他PLATFORM_*表示通信错误。
 */
static int emm_v5_set_position(struct motor *base, int32_t pulse)
{
  return emm_v5_move_relative(
      container_of(base, struct emm_v5, base), pulse);
}

/* EMM V5的公共生命周期、速度和位置能力表。 */
static const struct motor_ops s_ops = {
  .start = emm_v5_start,
  .stop = emm_v5_stop,
};

static const struct motor_speed_ops s_speed_ops = {
  .set_speed = emm_v5_set_speed,
  .get_speed = NULL,
};

static const struct motor_position_ops s_position_ops = {
  .set_position = emm_v5_set_position,
  .get_position = NULL,
};

/**
 * @brief  绑定EMM V5具体对象与motor基础能力表。
 * @param  self 调用方持有的EMM V5对象。
 * @param  name 逻辑名称，生命周期必须长于self。
 * @param  uart 已创建的抽象UART对象。
 * @param  config 电机地址、位置速度、加速度和超时配置。
 * @retval PLATFORM_OK表示绑定成功，其他PLATFORM_*表示参数或状态错误。
 */
int emm_v5_init(struct emm_v5 *self,
                const char *name,
                struct uart *uart,
                const struct emm_v5_config *config)
{
  int result;

  if ((self == NULL) || (name == NULL) || (uart == NULL) ||
      (config == NULL) || (config->address == 0U) ||
      (config->position_speed_rpm == 0U) ||
      (config->position_speed_rpm > EMM_V5_SPEED_RPM_MAX) ||
      (config->timeout_ms == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  result = motor_bind(&self->base, name, &s_ops, NULL,
                      &s_speed_ops, &s_position_ops);
  if (result != PLATFORM_OK) {
    return result;
  }
  self->uart = uart;
  self->config = *config;
  self->commanded_speed_rpm = 0;
  self->commanded_position_pulse = 0;
  return PLATFORM_OK;
}

/**
 * @brief  获取EMM V5对应的motor基础对象。
 * @param  self EMM V5具体对象。
 * @retval self有效时返回其motor基础对象，否则返回NULL。
 */
struct motor *emm_v5_get_motor(struct emm_v5 *self)
{
  return (self != NULL) ? &self->base : NULL;
}

int emm_v5_enabled_set(struct emm_v5 *self, bool enabled)
{
  uint8_t enable_frame[6];
  int result;

  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  enable_frame[0] = self->config.address;
  enable_frame[1] = EMM_V5_COMMAND_ENABLE;
  enable_frame[2] = EMM_V5_ENABLE_MAGIC;
  enable_frame[3] = enabled ? UINT8_C(1) : UINT8_C(0);
  enable_frame[4] = EMM_V5_SYNC_DISABLED;
  enable_frame[5] = EMM_V5_FRAME_CHECKSUM;
  return emm_v5_exchange(self, enable_frame, sizeof(enable_frame));
}

/**
 * @brief  设置EMM V5速度模式的目标转速。
 * @param  self 已启动的EMM V5对象。
 * @param  speed_rpm 范围-5000至5000，符号表示方向，单位为RPM。
 * @retval PLATFORM_OK表示命令发送成功，其他PLATFORM_*表示状态或通信错误。
 */
int emm_v5_set_speed_rpm(struct emm_v5 *self, int32_t speed_rpm)
{
  uint8_t frame[8];
  uint8_t direction;
  uint16_t magnitude_rpm;
  int result;

  if ((speed_rpm < -EMM_V5_SPEED_RPM_MAX) ||
      (speed_rpm > EMM_V5_SPEED_RPM_MAX)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  emm_v5_speed_encode(self, speed_rpm, &direction, &magnitude_rpm);
  frame[0] = self->config.address;
  frame[1] = EMM_V5_COMMAND_SPEED;
  frame[2] = direction;
  frame[3] = (uint8_t)(magnitude_rpm >> 8);
  frame[4] = (uint8_t)magnitude_rpm;
  frame[5] = self->config.acceleration;
  frame[6] = EMM_V5_SYNC_DISABLED;
  frame[7] = EMM_V5_FRAME_CHECKSUM;
  result = emm_v5_exchange(self, frame, sizeof(frame));
  if (result == PLATFORM_OK) {
    self->commanded_speed_rpm = speed_rpm;
  }
  return result;
}

/**
 * @brief  以标准位置模式移动指定的相对脉冲数。
 * @param  self 已启动的EMM V5对象。
 * @param  pulse 有符号相对脉冲数，正负方向受reversed配置影响。
 * @retval PLATFORM_OK表示命令发送成功，其他PLATFORM_*表示状态或通信错误。
 */
static int emm_v5_move_position(struct emm_v5 *self,
                                int32_t pulse,
                                uint8_t position_mode)
{
  uint8_t frame[13];
  uint8_t direction;
  uint32_t magnitude;
  bool ccw;
  int result;

  if ((position_mode != EMM_V5_POSITION_RELATIVE) &&
      (position_mode != EMM_V5_POSITION_ABSOLUTE)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  ccw = pulse < 0;
  if (pulse == INT32_MIN) {
    magnitude = (uint32_t)INT32_MAX + UINT32_C(1);
  } else {
    magnitude = (uint32_t)(ccw ? -pulse : pulse);
  }
  if (self->config.reversed) {
    ccw = !ccw;
  }
  direction = ccw ? EMM_V5_DIRECTION_CCW : EMM_V5_DIRECTION_CW;

  frame[0] = self->config.address;
  frame[1] = EMM_V5_COMMAND_POSITION;
  frame[2] = direction;
  frame[3] = (uint8_t)(self->config.position_speed_rpm >> 8);
  frame[4] = (uint8_t)self->config.position_speed_rpm;
  frame[5] = self->config.acceleration;
  frame[6] = (uint8_t)(magnitude >> 24);
  frame[7] = (uint8_t)(magnitude >> 16);
  frame[8] = (uint8_t)(magnitude >> 8);
  frame[9] = (uint8_t)magnitude;
  frame[10] = position_mode;
  frame[11] = EMM_V5_SYNC_DISABLED;
  frame[12] = EMM_V5_FRAME_CHECKSUM;
  result = emm_v5_exchange(self, frame, sizeof(frame));
  if (result == PLATFORM_OK) {
    self->commanded_position_pulse = pulse;
  }
  return result;
}

int emm_v5_move_relative(struct emm_v5 *self, int32_t pulse)
{
  return emm_v5_move_position(self, pulse, EMM_V5_POSITION_RELATIVE);
}

int emm_v5_move_absolute(struct emm_v5 *self, int32_t pulse)
{
  return emm_v5_move_position(self, pulse, EMM_V5_POSITION_ABSOLUTE);
}

int emm_v5_position_clear(struct emm_v5 *self)
{
  uint8_t frame[4];
  int result;

  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  frame[0] = self->config.address;
  frame[1] = EMM_V5_COMMAND_CLEAR_POSITION;
  frame[2] = EMM_V5_CLEAR_POSITION_MAGIC;
  frame[3] = EMM_V5_FRAME_CHECKSUM;
  result = emm_v5_exchange(self, frame, sizeof(frame));
  if (result == PLATFORM_OK) {
    self->commanded_position_pulse = 0;
  }
  return result;
}

/**
 * @brief  立即停止EMM V5当前运动。
 * @param  self 已启动的EMM V5对象。
 * @retval PLATFORM_OK表示停止命令发送成功，其他PLATFORM_*表示状态或通信错误。
 */
int emm_v5_stop_now(struct emm_v5 *self)
{
  uint8_t frame[5];
  int result;

  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  frame[0] = self->config.address;
  frame[1] = EMM_V5_COMMAND_STOP_NOW;
  frame[2] = EMM_V5_STOP_MAGIC;
  frame[3] = EMM_V5_SYNC_DISABLED;
  frame[4] = EMM_V5_FRAME_CHECKSUM;
  result = emm_v5_exchange(self, frame, sizeof(frame));
  if (result == PLATFORM_OK) {
    self->commanded_speed_rpm = 0;
  }
  return result;
}

/**
 * @brief  通过串口读取EMM V5编码器实时位置。
 * @param  self 已启动的EMM V5对象。
 * @param  position_raw 用于接收实时位置原始值的变量。
 * @retval PLATFORM_OK表示读取成功，其他PLATFORM_*表示状态或通信错误。
 * @note   原始位置每65536单位表示电机旋转一圈，符号表示驱动器方向。
 */
int emm_v5_read_position(struct emm_v5 *self,
                         int32_t *position_raw)
{
  uint8_t frame[3];
  uint8_t response[EMM_V5_POSITION_RESPONSE_SIZE];
  uint32_t magnitude;
  int result;

  if (position_raw == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  result = emm_v5_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  frame[0] = self->config.address;
  frame[1] = EMM_V5_COMMAND_READ_POSITION;
  frame[2] = EMM_V5_FRAME_CHECKSUM;
  delay_ms(EMM_V5_COMMAND_PRE_DELAY_MS);
  result = uart_write(self->uart, frame, sizeof(frame),
                      self->config.timeout_ms);
  if (result == PLATFORM_OK) {
    result = uart_blocking_read(self->uart, response,
                                EMM_V5_POSITION_RESPONSE_SIZE,
                                self->config.timeout_ms);
  }
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((response[0] != self->config.address) ||
      (response[1] != EMM_V5_COMMAND_READ_POSITION) ||
      (response[7] != EMM_V5_FRAME_CHECKSUM)) {
    return PLATFORM_ERR_IO;
  }

  magnitude = ((uint32_t)response[3] << 24U) |
              ((uint32_t)response[4] << 16U) |
              ((uint32_t)response[5] << 8U) |
              (uint32_t)response[6];
  if (response[2] == EMM_V5_POSITION_SIGN_POSITIVE) {
    if (magnitude > (uint32_t)INT32_MAX) {
      return PLATFORM_ERR_IO;
    }
    *position_raw = (int32_t)magnitude;
  } else if (response[2] == EMM_V5_POSITION_SIGN_NEGATIVE) {
    if (magnitude > ((uint32_t)INT32_MAX + UINT32_C(1))) {
      return PLATFORM_ERR_IO;
    }
    *position_raw = (magnitude ==
                     ((uint32_t)INT32_MAX + UINT32_C(1))) ?
                    INT32_MIN : -(int32_t)magnitude;
  } else {
    return PLATFORM_ERR_IO;
  }
  return PLATFORM_OK;
}
