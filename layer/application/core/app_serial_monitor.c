/* SPDX-License-Identifier: MIT */
/**
 * @file    app_serial_monitor.c
 * @brief   WEIXIN Serial Monitor UART1 通信任务实现。
 * @author  weixin
 *
 * UART1 同时承载 printf 日志和上位机协议。协议响应使用单次 uart_write()
 * 完整发送，并在前面补充 CRLF，防止普通日志片段破坏 [param] 行首。
 */
#include "app_serial_monitor.h"

#include "app_ball_control.h"
#include "app_system_stats.h"

#include "board_drive.h"
#include "emm_v5/board_emm_v5.h"
#include "board_uart.h"
#include "platform/inferface/uart/uart_interrupt.h"
#include "result.h"
#include "weixin_protocol/weixin_protocol.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* 固件版本编码为 major * 10000 + minor * 100 + patch。 */
#define SERIAL_MONITOR_FIRMWARE_VERSION UINT32_C(10000)
/* UART 单次协议响应的发送超时，单位为毫秒。 */
#define SERIAL_MONITOR_TX_TIMEOUT_MS UINT32_C(250)
#define SERIAL_MONITOR_TX_RETRY_COUNT UINT32_C(5)
/* UART 忙重试间隔，单位为毫秒。 */
#define SERIAL_MONITOR_TX_RETRY_DELAY_MS UINT32_C(1)
/* UART中断接收链续接失败后的重试间隔，单位为毫秒。 */
#define SERIAL_MONITOR_RX_ERROR_DELAY_MS UINT32_C(10)
/* 接收事件队列长度；在115200 baud下可缓存约11 ms的连续数据。 */
#define SERIAL_MONITOR_RX_QUEUE_LENGTH ((UBaseType_t)128U)
/* 协议响应前置 CRLF 的字节数量。 */
#define SERIAL_MONITOR_LINE_PREFIX_SIZE UINT16_C(2)
/* 应用层响应缓冲区包含前置 CRLF 和协议组件最大响应。 */
#define SERIAL_MONITOR_TX_CAPACITY \
  (WEIXIN_PROTOCOL_RESPONSE_CAPACITY + SERIAL_MONITOR_LINE_PREFIX_SIZE)
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
/* 上位机允许写入的速度 PI 增益上限。 */
#define SERIAL_MONITOR_SPEED_GAIN_MAX (10.0)
/* 上位机允许设置的单轮基础速度绝对值，单位为编码器计数每秒。 */
#define SERIAL_MONITOR_DRIVE_SPEED_MAX INT32_C(200000)
#endif
/* 左右轮默认基础速度，保证默认平均目标速度为 80000 count/s。 */
#define SERIAL_MONITOR_DRIVE_SPEED_DEFAULT INT32_C(60000)
/* 小球位置环允许通过WEIXIN设置的增益范围。 */
#define SERIAL_MONITOR_BALL_POSITION_GAIN_MIN (-100.0)
#define SERIAL_MONITOR_BALL_POSITION_GAIN_MAX (100.0)
#if SERIAL_MONITOR_WAVE_ENABLE
/* WEIXIN所有已启用波形通道的发送周期，单位为毫秒。 */
#define SERIAL_MONITOR_WAVE_PERIOD_MS UINT32_C(100)
/* 单行所有已启用波形通道的文本缓冲区容量。 */
#define SERIAL_MONITOR_WAVE_CAPACITY UINT16_C(256)
#define SERIAL_MONITOR_RX_WAIT_TICKS \
  pdMS_TO_TICKS(SERIAL_MONITOR_WAVE_PERIOD_MS)
#else
#define SERIAL_MONITOR_RX_WAIT_TICKS portMAX_DELAY
#endif

/* 上位机可读取的固件版本。 */
static uint32_t s_firmware_version = SERIAL_MONITOR_FIRMWARE_VERSION;
/* 上位机请求的左右速度环比例增益，由电机任务在周期边界应用。 */
static float s_drive_speed_kp = BOARD_DRIVE_SPEED_KP_DEFAULT;
/* 上位机请求的左右速度环积分增益，由电机任务在周期边界应用。 */
static float s_drive_speed_ki = BOARD_DRIVE_SPEED_KI_DEFAULT;
/* 循迹任务读取的左轮基础速度，单位为编码器计数每秒。 */
static int32_t s_drive_left_speed = SERIAL_MONITOR_DRIVE_SPEED_DEFAULT;
/* 循迹任务读取的右轮基础速度，单位为编码器计数每秒。 */
static int32_t s_drive_right_speed = SERIAL_MONITOR_DRIVE_SPEED_DEFAULT;
/* WEIXIN设置的小球位置环增益，由球控任务在周期边界应用。 */
static float s_ball_position_kp = BALL_CONTROL_POSITION_KP_DEFAULT;
static float s_ball_position_ki = BALL_CONTROL_POSITION_KI_DEFAULT;
static float s_ball_position_kd = BALL_CONTROL_POSITION_KD_DEFAULT;
/* 上位机请求的EMM绝对位置，单位为度。 */
static float s_emm_position_deg;
/* UART 发送最终失败次数，只由本任务更新。 */
static uint32_t s_tx_error_count;
/* UART 非超时接收失败次数，只由本任务更新。 */
static uint32_t s_rx_error_count;
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
/* 电机任务发布的左轮实测速度快照，单位为编码器计数每秒。 */
static int32_t s_wave_left_speed;
/* 电机任务发布的右轮实测速度快照，单位为编码器计数每秒。 */
static int32_t s_wave_right_speed;
/* 标记电机任务是否已经发布过有效速度快照。 */
static bool s_wave_speed_valid;
#endif
#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
/* 球控任务发布的视觉小球原始速度快照，单位为像素每帧。 */
static int8_t s_wave_ball_velocity;
/* 球控任务发布的小球低通滤波速度快照，单位为像素每帧。 */
static float s_wave_ball_filtered_velocity;
/* 标记球控任务是否已经发布过有效小球速度快照。 */
static bool s_wave_ball_velocity_valid;
/* 球控PID最近一次写入EMM邮箱的整数目标速度，单位为RPM。 */
static int32_t s_wave_emm_speed_rpm;
/* 球控目标默认静止，先发布0以便上位机启动时立即注册该波形通道。 */
static bool s_wave_emm_speed_valid = true;
#endif
#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
/* 球控任务发布的位置环目标和视觉测量位置，单位为像素。 */
static int16_t s_wave_ball_target_x;
static int16_t s_wave_ball_position_x;
static float s_wave_ball_filtered_x;
static bool s_wave_ball_position_valid;
#endif
/* WEIXIN 协议组件对象，只由串口通信任务访问。 */
static struct weixin_protocol s_protocol;
/* 单次 UART 写入使用的完整响应缓冲区。 */
static uint8_t s_tx_buffer[SERIAL_MONITOR_TX_CAPACITY];
#if SERIAL_MONITOR_WAVE_ENABLE
/* 串口任务组装 WEIXIN 波形行时使用的静态缓冲区。 */
static char s_wave_buffer[SERIAL_MONITOR_WAVE_CAPACITY];
#endif
/* 中断每次接收一个字节时使用的静态存储。 */
static uint8_t s_rx_byte;
/* ISR向串口任务传递接收结果的FreeRTOS队列。 */
static QueueHandle_t s_rx_queue;
static volatile bool s_rx_queue_overflow;
static volatile bool s_rx_rearm_required;

struct serial_monitor_rx_event {
  uint8_t byte;
  int8_t result;
};
/* 默认协议向上位机公开的安全参数表。 */
static const struct weixin_param s_params[] = {
  {
    .name = "system.fw_version",
    .type = WEIXIN_PARAM_TYPE_U32,
    .value = &s_firmware_version,
    .minimum = 0.0,
    .maximum = 4294967295.0,
    .writable = false,
  },
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
  {
    .name = "drive.speed_kp",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_drive_speed_kp,
    .minimum = 0.0,
    .maximum = SERIAL_MONITOR_SPEED_GAIN_MAX,
    .writable = true,
  },
  {
    .name = "drive.left_speed",
    .type = WEIXIN_PARAM_TYPE_I32,
    .value = &s_drive_left_speed,
    .minimum = -SERIAL_MONITOR_DRIVE_SPEED_MAX,
    .maximum = SERIAL_MONITOR_DRIVE_SPEED_MAX,
    .writable = true,
  },
  {
    .name = "drive.right_speed",
    .type = WEIXIN_PARAM_TYPE_I32,
    .value = &s_drive_right_speed,
    .minimum = -SERIAL_MONITOR_DRIVE_SPEED_MAX,
    .maximum = SERIAL_MONITOR_DRIVE_SPEED_MAX,
    .writable = true,
  },
  {
    .name = "drive.speed_ki",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_drive_speed_ki,
    .minimum = 0.0,
    .maximum = SERIAL_MONITOR_SPEED_GAIN_MAX,
    .writable = true,
  },
#endif
  {
    .name = "ball.position_kp",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_ball_position_kp,
    .minimum = SERIAL_MONITOR_BALL_POSITION_GAIN_MIN,
    .maximum = SERIAL_MONITOR_BALL_POSITION_GAIN_MAX,
    .writable = true,
  },
  {
    .name = "ball.position_ki",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_ball_position_ki,
    .minimum = SERIAL_MONITOR_BALL_POSITION_GAIN_MIN,
    .maximum = SERIAL_MONITOR_BALL_POSITION_GAIN_MAX,
    .writable = true,
  },
  {
    .name = "ball.position_kd",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_ball_position_kd,
    .minimum = SERIAL_MONITOR_BALL_POSITION_GAIN_MIN,
    .maximum = SERIAL_MONITOR_BALL_POSITION_GAIN_MAX,
    .writable = true,
  },
  {
    .name = "emm.position_deg",
    .type = WEIXIN_PARAM_TYPE_FLOAT,
    .value = &s_emm_position_deg,
    .minimum = BOARD_EMM_V5_POSITION_DEG_MIN,
    .maximum = BOARD_EMM_V5_POSITION_DEG_MAX,
    .writable = true,
  },
  {
    .name = "monitor.tx_errors",
    .type = WEIXIN_PARAM_TYPE_U32,
    .value = &s_tx_error_count,
    .minimum = 0.0,
    .maximum = 4294967295.0,
    .writable = false,
  },
  {
    .name = "monitor.rx_errors",
    .type = WEIXIN_PARAM_TYPE_U32,
    .value = &s_rx_error_count,
    .minimum = 0.0,
    .maximum = 4294967295.0,
    .writable = false,
  },
};

/**
 * @brief  获取上位机请求的电机速度 PI 参数快照。
 * @param  speed_kp 用于接收比例增益的变量。
 * @param  speed_ki 用于接收积分增益的变量。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   临界区确保控制任务读取两个 32 位浮点值时不会被任务切换打断。
 */
int serial_monitor_drive_speed_gains_get(float *speed_kp,
                                         float *speed_ki)
{
  if ((speed_kp == NULL) || (speed_ki == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  *speed_kp = s_drive_speed_kp;
  *speed_ki = s_drive_speed_ki;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

int serial_monitor_ball_position_gains_get(float *position_kp,
                                           float *position_ki,
                                           float *position_kd)
{
  if ((position_kp == NULL) || (position_ki == NULL) ||
      (position_kd == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  *position_kp = s_ball_position_kp;
  *position_ki = s_ball_position_ki;
  *position_kd = s_ball_position_kd;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

/**
 * @brief  获取上位机配置的左右轮循迹基础速度快照。
 * @param  left_speed 用于接收左轮基础速度，单位为编码器计数每秒。
 * @param  right_speed 用于接收右轮基础速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   临界区确保循迹任务读取到同一时刻的左右基础速度。
 */
int serial_monitor_drive_base_speeds_get(int32_t *left_speed,
                                         int32_t *right_speed)
{
  if ((left_speed == NULL) || (right_speed == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  *left_speed = s_drive_left_speed;
  *right_speed = s_drive_right_speed;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

/**
 * @brief  获取上位机请求的EMM绝对位置快照。
 * @param  position_deg 用于接收目标角度的变量。
 * @return PLATFORM_OK表示读取成功，参数为空时返回参数错误。
 * @note   本函数只读取目标值，不访问EMM或USART6硬件。
 */
int serial_monitor_emm_position_deg_get(float *position_deg)
{
  if (position_deg == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  *position_deg = s_emm_position_deg;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
/**
 * @brief  发布左右轮实测速度，供 WEIXIN 波形协议低频发送。
 * @param  left_speed 左轮实测速度，单位为编码器计数每秒。
 * @param  right_speed 右轮实测速度，单位为编码器计数每秒。
 * @retval None。
 * @note   临界区只保护两个 32 位速度值和有效标志，不执行串口操作。
 */
void serial_monitor_drive_speeds_publish(int32_t left_speed,
                                         int32_t right_speed)
{
  taskENTER_CRITICAL();
  s_wave_left_speed = left_speed;
  s_wave_right_speed = right_speed;
  s_wave_speed_valid = true;
  taskEXIT_CRITICAL();
}

/**
 * @brief  获取最近一次左右轮实测速度快照。
 * @param  left_speed 用于接收左轮实测速度的变量。
 * @param  right_speed 用于接收右轮实测速度的变量。
 * @return true 表示存在有效快照，false 表示电机任务尚未发布速度。
 */
static bool serial_monitor_drive_speeds_snapshot_get(
    int32_t *left_speed,
    int32_t *right_speed)
{
  bool valid;

  taskENTER_CRITICAL();
  valid = s_wave_speed_valid;
  *left_speed = s_wave_left_speed;
  *right_speed = s_wave_right_speed;
  taskEXIT_CRITICAL();
  return valid;
}
#endif

#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
/**
 * @brief  发布小球原始速度与低通结果，供WEIXIN波形协议发送。
 * @param  velocity 小球原始速度，单位为像素每帧。
 * @param  filtered_velocity 小球低通滤波速度，单位为像素每帧。
 * @retval None。
 * @note   临界区只保护速度快照和有效标志，不执行串口操作。
 */
void serial_monitor_ball_velocities_publish(int8_t velocity,
                                            float filtered_velocity)
{
  taskENTER_CRITICAL();
  s_wave_ball_velocity = velocity;
  s_wave_ball_filtered_velocity = filtered_velocity;
  s_wave_ball_velocity_valid = true;
  taskEXIT_CRITICAL();
}

/**
 * @brief  获取最近一次视觉小球原始速度快照。
 * @param  velocity 用于接收小球原始速度的变量，单位为像素每帧。
 * @param  filtered_velocity 用于接收低通速度的变量，单位为像素每帧。
 * @return true表示存在有效快照，false表示球控任务尚未发布。
 */
static bool serial_monitor_ball_velocities_snapshot_get(
    int8_t *velocity,
    float *filtered_velocity)
{
  bool valid;

  taskENTER_CRITICAL();
  valid = s_wave_ball_velocity_valid;
  *velocity = s_wave_ball_velocity;
  *filtered_velocity = s_wave_ball_filtered_velocity;
  taskEXIT_CRITICAL();
  return valid;
}

/**
 * @brief  发布球控PID写入EMM邮箱的整数目标速度，供WEIXIN波形协议发送。
 * @param  speed_rpm PID浮点输出取整后的目标速度，单位为RPM。
 * @retval None。
 * @note   临界区只保护速度快照和有效标志，不执行串口操作。
 */
void serial_monitor_ball_speed_rpm_publish(int32_t speed_rpm)
{
  taskENTER_CRITICAL();
  s_wave_emm_speed_rpm = speed_rpm;
  s_wave_emm_speed_valid = true;
  taskEXIT_CRITICAL();
}

/**
 * @brief  获取最近一次球控PID写入EMM邮箱的整数目标速度快照。
 * @param  speed_rpm 用于接收目标速度的变量，单位为RPM。
 * @return true表示存在有效快照，false表示EMM任务尚未发布。
 */
static bool serial_monitor_emm_speed_snapshot_get(int32_t *speed_rpm)
{
  bool valid;

  taskENTER_CRITICAL();
  valid = s_wave_emm_speed_valid;
  *speed_rpm = s_wave_emm_speed_rpm;
  taskEXIT_CRITICAL();
  return valid;
}
#endif

#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
/**
 * @brief  发布小球位置环快照，供WEIXIN波形协议发送。
 * @param  target_x 目标位置，单位为像素。
 * @param  measured_x 视觉测量位置，单位为像素。
 * @param  filtered_x 低通滤波后的位置，单位为像素。
 * @retval None。
 */
void serial_monitor_ball_positions_publish(int16_t target_x,
                                           int16_t measured_x,
                                           float filtered_x)
{
  taskENTER_CRITICAL();
  s_wave_ball_target_x = target_x;
  s_wave_ball_position_x = measured_x;
  s_wave_ball_filtered_x = filtered_x;
  s_wave_ball_position_valid = true;
  taskEXIT_CRITICAL();
}

static bool serial_monitor_ball_positions_snapshot_get(
    int16_t *target_x,
    int16_t *measured_x,
    float *filtered_x)
{
  bool valid;

  taskENTER_CRITICAL();
  valid = s_wave_ball_position_valid;
  *target_x = s_wave_ball_target_x;
  *measured_x = s_wave_ball_position_x;
  *filtered_x = s_wave_ball_filtered_x;
  taskEXIT_CRITICAL();
  return valid;
}
#endif

/**
 * @brief  饱和递增 32 位错误计数器。
 * @param  counter：待更新的错误计数器。
 * @retval None。
 */
static void serial_monitor_error_increment(uint32_t *counter)
{
  if (*counter < UINT32_MAX) {
    (*counter)++;
  }
}

static bool serial_monitor_isr_flag_take(volatile bool *flag)
{
  bool value;

  taskENTER_CRITICAL();
  value = *flag;
  *flag = false;
  taskEXIT_CRITICAL();
  return value;
}

static void serial_monitor_rx_callback(struct uart *self,
                                       uint8_t *data,
                                       uint16_t size,
                                       int result,
                                       void *context)
{
  struct serial_monitor_rx_event event;
  BaseType_t higher_priority_task_woken = pdFALSE;
  int rearm_result;

  (void)context;
  event.byte = UINT8_C(0);
  event.result = (int8_t)result;
  if ((result == PLATFORM_OK) && (data != NULL) &&
      (size == UINT16_C(1))) {
    event.byte = data[0];
  } else if (result == PLATFORM_OK) {
    event.result = (int8_t)PLATFORM_ERR_IO;
  }

  rearm_result = uart_interrupt_receive(
      self, &s_rx_byte, UINT16_C(1), serial_monitor_rx_callback, NULL);
  if (rearm_result != PLATFORM_OK) {
    s_rx_rearm_required = true;
  }
  if ((s_rx_queue == NULL) ||
      (xQueueSendFromISR(s_rx_queue, &event,
                         &higher_priority_task_woken) != pdPASS)) {
    s_rx_queue_overflow = true;
  }
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void serial_monitor_receive_rearm(struct uart *uart)
{
  int result;

  do {
    result = uart_interrupt_receive(
        uart, &s_rx_byte, UINT16_C(1), serial_monitor_rx_callback, NULL);
    if (result != PLATFORM_OK) {
      serial_monitor_error_increment(&s_rx_error_count);
      vTaskDelay(pdMS_TO_TICKS(SERIAL_MONITOR_RX_ERROR_DELAY_MS));
    }
  } while (result != PLATFORM_OK);
}

/**
 * @brief  通过 UART1 单次写入完整协议响应。
 * @param  context：由协议组件传入的 UART 基础对象。
 * @param  data：不包含 NUL 的协议响应。
 * @param  size：协议响应长度。
 * @return PLATFORM_OK 表示发送成功；其他 PLATFORM_* 表示最终失败。
 * @note   前置 CRLF 用于隔离其他任务可能正在输出的普通日志片段。
 */
static int serial_monitor_write(void *context,
                                const uint8_t *data,
                                uint16_t size)
{
  struct uart *uart = (struct uart *)context;
  uint32_t retry;
  uint16_t total_size;
  int result;

  if ((uart == NULL) || (data == NULL) ||
      (size > WEIXIN_PROTOCOL_RESPONSE_CAPACITY)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  s_tx_buffer[0] = '\r';
  s_tx_buffer[1] = '\n';
  memcpy(&s_tx_buffer[SERIAL_MONITOR_LINE_PREFIX_SIZE], data, size);
  total_size = (uint16_t)(size + SERIAL_MONITOR_LINE_PREFIX_SIZE);

  for (retry = 0U; retry < SERIAL_MONITOR_TX_RETRY_COUNT; retry++) {
    result = uart_write(uart, s_tx_buffer, total_size,
                        SERIAL_MONITOR_TX_TIMEOUT_MS);
    if (result != PLATFORM_ERR_BUSY) {
      if (result != PLATFORM_OK) {
        serial_monitor_error_increment(&s_tx_error_count);
      }
      return result;
    }
    vTaskDelay(pdMS_TO_TICKS(SERIAL_MONITOR_TX_RETRY_DELAY_MS));
  }

  serial_monitor_error_increment(&s_tx_error_count);
  return PLATFORM_ERR_BUSY;
}

/**
 * @brief  通过USART1发送一组已启用的WEIXIN波形采样。
 * @param  uart 已初始化并绑定到 USART1 的 UART 对象。
 * @return PLATFORM_OK 表示发送成功或尚无有效采样，其他 PLATFORM_* 表示失败。
 * @note   所有有效通道位于同一行，因此GUI使用同一采样序号绘制它们。
 */
#if SERIAL_MONITOR_WAVE_ENABLE
static int serial_monitor_wave_report(struct uart *uart)
{
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
  int32_t left_speed;
  int32_t right_speed;
#endif
#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
  int8_t ball_velocity;
  float ball_filtered_velocity;
  int32_t emm_speed_rpm;
#endif
#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
  int16_t ball_target_x;
  int16_t ball_position_x;
  float ball_filtered_x;
#endif
  int written;
  int appended;

  written = 0;
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
  if (serial_monitor_drive_speeds_snapshot_get(
          &left_speed, &right_speed)) {
    written = snprintf(
        s_wave_buffer, sizeof(s_wave_buffer),
        "wave.left_speed=%ld wave.right_speed=%ld",
        (long)left_speed, (long)right_speed);
    if ((written < 0) || (written >= (int)sizeof(s_wave_buffer))) {
      return PLATFORM_ERR_IO;
    }
  }
#endif

#if (SERIAL_MONITOR_BALL_SPEED_WAVE_ENABLE == 1)
  if (serial_monitor_ball_velocities_snapshot_get(
          &ball_velocity, &ball_filtered_velocity)) {
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_velocity=%d wave.ball_velocity_filtered=%.3f",
        (written > 0) ? " " : "",
        (int)ball_velocity,
        (double)ball_filtered_velocity);
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
  }
  if (serial_monitor_emm_speed_snapshot_get(&emm_speed_rpm)) {
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_speed_rpm=%ld",
        (written > 0) ? " " : "",
        (long)emm_speed_rpm);
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
  }
#endif
#if (SERIAL_MONITOR_BALL_POSITION_WAVE_ENABLE == 1)
  if (serial_monitor_ball_positions_snapshot_get(
          &ball_target_x, &ball_position_x, &ball_filtered_x)) {
#if (SERIAL_MONITOR_BALL_TARGET_X_WAVE_ENABLE == 1)
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_target_x=%d",
        (written > 0) ? " " : "",
        (int)ball_target_x);
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
#endif
#if (SERIAL_MONITOR_BALL_POSITION_X_WAVE_ENABLE == 1)
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_position_x=%d",
        (written > 0) ? " " : "",
        (int)ball_position_x);
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
#endif
#if (SERIAL_MONITOR_BALL_POSITION_X_FILTERED_WAVE_ENABLE == 1)
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_position_x_filtered=%.3f",
        (written > 0) ? " " : "",
        (double)ball_filtered_x);
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
#endif
#if (SERIAL_MONITOR_BALL_ERROR_X_WAVE_ENABLE == 1)
    appended = snprintf(
        s_wave_buffer + written,
        sizeof(s_wave_buffer) - (size_t)written,
        "%swave.ball_error_x=%.3f",
        (written > 0) ? " " : "",
        (double)((float)ball_target_x - ball_filtered_x));
    if ((appended < 0) ||
        (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
      return PLATFORM_ERR_IO;
    }
    written += appended;
#endif
  }
#endif
  if (written == 0) {
    return PLATFORM_OK;
  }
  appended = snprintf(s_wave_buffer + written,
                      sizeof(s_wave_buffer) - (size_t)written,
                      "\r\n");
  if ((appended != 2) ||
      (appended >= (int)(sizeof(s_wave_buffer) - (size_t)written))) {
    return PLATFORM_ERR_IO;
  }
  written += appended;

  return serial_monitor_write(
      uart, (const uint8_t *)s_wave_buffer, (uint16_t)written);
}
#endif

/**
 * @brief  处理串口助手发送的应用自定义文本命令。
 * @param  context：应用命令上下文，当前实现不使用。
 * @param  argument_count：命令行参数数量。
 * @param  arguments：按空格或 Tab 切分后的只读参数数组。
 * @retval true：识别到 `system status` 并已请求生成系统状态报告。
 * @retval false：命令不属于本应用，由协议组件继续处理或静默忽略。
 */
static bool serial_monitor_command_handle(
    void *context,
    uint8_t argument_count,
    const char *const *arguments)
{
  (void)context;

  if ((argument_count == UINT8_C(2)) &&
      (strcmp(arguments[0], "system") == 0) &&
      (strcmp(arguments[1], "status") == 0)) {
    system_stats_report_request();
    return true;
  }

  return false;
}

/**
 * @brief  初始化协议组件并启动UART1单字节中断接收。
 * @param  uart：用于接收上位机命令的 UART1 对象。
 * @return PLATFORM_OK 表示准备成功；其他 PLATFORM_* 表示失败。
 */
static int serial_monitor_init(struct uart *uart)
{
  int result;

  if ((uart == NULL) || (uart->interrupt_ops == NULL)) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  s_rx_queue = xQueueCreate(
      SERIAL_MONITOR_RX_QUEUE_LENGTH,
      (UBaseType_t)sizeof(struct serial_monitor_rx_event));
  if (s_rx_queue == NULL) {
    return PLATFORM_ERR_IO;
  }

  result = weixin_protocol_init(
      &s_protocol,
      s_params,
      (uint16_t)(sizeof(s_params) / sizeof(s_params[0])),
      serial_monitor_write,
      uart,
      serial_monitor_command_handle,
      NULL);
  if (result == PLATFORM_OK) {
    result = uart_interrupt_receive(
        uart, &s_rx_byte, UINT16_C(1), serial_monitor_rx_callback, NULL);
  }
  if (result != PLATFORM_OK) {
    vQueueDelete(s_rx_queue);
    s_rx_queue = NULL;
  }
  return result;
}

/**
 * @brief  运行 WEIXIN Serial Monitor UART1 通信任务。
 * @param  argument：FreeRTOS 任务参数，当前未使用。
 * @retval None。
 */
void serial_monitor_task(void *argument)
{
  struct uart *uart;
  struct serial_monitor_rx_event event;
#if SERIAL_MONITOR_WAVE_ENABLE
  TickType_t last_wave_time;
  TickType_t current_time;
#endif
  bool discarding_line = false;
  int result;

  (void)argument;
  uart = board_uart1_get();
  result = serial_monitor_init(uart);
  if (result != PLATFORM_OK) {
    printf("[monitor] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  printf("[monitor] ready, params=%u\r\n",
         (unsigned int)(sizeof(s_params) / sizeof(s_params[0])));
#if SERIAL_MONITOR_WAVE_ENABLE
  last_wave_time = xTaskGetTickCount();
#endif

  for (;;) {
    if (xQueueReceive(s_rx_queue, &event,
                      SERIAL_MONITOR_RX_WAIT_TICKS) == pdPASS) {
      if (serial_monitor_isr_flag_take(&s_rx_queue_overflow)) {
        (void)weixin_protocol_reset_line(&s_protocol);
        discarding_line = true;
        serial_monitor_error_increment(&s_rx_error_count);
      }
      if (event.result != (int8_t)PLATFORM_OK) {
        (void)weixin_protocol_reset_line(&s_protocol);
        discarding_line = true;
        serial_monitor_error_increment(&s_rx_error_count);
      } else if (discarding_line) {
        if ((event.byte == (uint8_t)'\r') ||
            (event.byte == (uint8_t)'\n')) {
          discarding_line = false;
        }
      } else {
        (void)weixin_protocol_feed_byte(&s_protocol, event.byte);
      }
    }

    if (serial_monitor_isr_flag_take(&s_rx_rearm_required)) {
      (void)weixin_protocol_reset_line(&s_protocol);
      discarding_line = true;
      serial_monitor_receive_rearm(uart);
    }

#if SERIAL_MONITOR_WAVE_ENABLE
    current_time = xTaskGetTickCount();
    if ((TickType_t)(current_time - last_wave_time) >=
        SERIAL_MONITOR_RX_WAIT_TICKS) {
      last_wave_time = current_time;
      (void)serial_monitor_wave_report(uart);
    }
#endif
  }
}
