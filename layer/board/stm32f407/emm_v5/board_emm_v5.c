/* SPDX-License-Identifier: MIT */
/**
 * @file    board_emm_v5.c
 * @brief   STM32F407板级EMM V5电机对象与USART6资源组合。
 * @author  weixin
 */
#include "board_emm_v5.h"

#include "emm_v5/emm_v5.h"
#include "board_uart.h"
#include "result.h"

#include <stdbool.h>
#include <stddef.h>

/* 板载EMM V5电机的串口设备地址。 */
#define BOARD_EMM_V5_ADDRESS UINT8_C(1)
/* 快速位置模式的默认运行速度，单位为RPM。 */
#define BOARD_EMM_V5_POSITION_SPEED_RPM UINT16_C(300)
/* 0表示由EMM V5驱动器直接起动，不额外做加速度斜坡。 */
#define BOARD_EMM_V5_ACCELERATION UINT8_C(0)
/* EMM V5命令发送和四字节应答接收的单次超时，单位为ms。 */
#define BOARD_EMM_V5_TIMEOUT_MS UINT32_C(500)

/* 板级静态EMM V5具体对象，其生命周期覆盖整个固件运行期。 */
static struct emm_v5 s_emm_v5;
/* 防止重复执行motor_bind()造成对象重复绑定。 */
static bool s_emm_v5_bound;

/**
 * @brief  绑定板载EMM V5电机对象和其USART6通信资源。
 * @retval PLATFORM_OK表示绑定成功或已经绑定，其他PLATFORM_*表示失败。
 * @note   USART6采用PC6作为TX、PC7作为RX；Board层启动UART，
 *         EMM V5协议驱动在motor_start()时发送电机使能命令。
 */
int board_emm_v5_init(void)
{
  static const struct emm_v5_config config = {
    .address = BOARD_EMM_V5_ADDRESS,
    .position_speed_rpm = BOARD_EMM_V5_POSITION_SPEED_RPM,
    .acceleration = BOARD_EMM_V5_ACCELERATION,
    .timeout_ms = BOARD_EMM_V5_TIMEOUT_MS,
    /* 当前驱动器未返回控制命令应答，按官方上位机的只发送模式运行。 */
    .response_required = false,
    .reversed = false,
  };
  struct uart *uart;
  int result;

  if (s_emm_v5_bound) {
    return PLATFORM_OK;
  }
  result = board_uart6_init();
  if (result != PLATFORM_OK) {
    return result;
  }
  uart = board_uart6_get();
  if (uart == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  result = emm_v5_init(&s_emm_v5, "emm_v5", uart, &config);
  if (result == PLATFORM_OK) {
    s_emm_v5_bound = true;
  }
  return result;
}

/**
 * @brief  获取板载EMM V5对应的motor基础对象。
 * @retval board_emm_v5_init()成功后返回motor对象，否则返回NULL。
 */
struct motor *board_emm_v5_get(void)
{
  return s_emm_v5_bound ? emm_v5_get_motor(&s_emm_v5) : NULL;
}
