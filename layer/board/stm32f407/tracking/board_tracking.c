/* SPDX-License-Identifier: MIT */
/**
 * @file    board_tracking.c
 * @brief   STM32F407 八路循迹模块 GPIO 输入配置。
 * @author  weixin
 */
#include "board_tracking.h"

#include "result.h"
#include "stm32f4.h"

#include <stdbool.h>
#include <stddef.h>

/*
 * 通道顺序与右排针物理位置一致：15、14、13、12、11、10、9、8。
 * 引脚分别为 PD7、PD5、PD3、PD1、PC12、PC10、PA12、PA8。
 */
static const struct tracking_inputs_config s_tracking_config = {
  .pins = {
    STM32F4_PIN_PD7,
    STM32F4_PIN_PD5,
    STM32F4_PIN_PD3,
    STM32F4_PIN_PD1,
    STM32F4_PIN_PC12,
    STM32F4_PIN_PC10,
    STM32F4_PIN_PA12,
    STM32F4_PIN_PA8,
  },
};

/* Board 层持有的八路循迹输入组件对象。 */
static struct tracking_inputs s_tracking_inputs;
/* 防止重复配置八路循迹输入。 */
static bool s_initialized;

/**
 * @brief  初始化右排针 15 至 8 号位的八路循迹输入。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU GPIO 后端初始化失败。
 * @note   所有输入均配置为带内部上拉的 GPIO 输入。
 */
int board_tracking_init(void)
{
  int result;

  if (s_initialized) {
    return PLATFORM_OK;
  }

  result = stm32f4_init();
  if (result != PLATFORM_OK) {
    return result;
  }

  result = tracking_inputs_init(&s_tracking_inputs,
                                &s_tracking_config);
  if (result == PLATFORM_OK) {
    s_initialized = true;
  }
  return result;
}

/**
 * @brief  读取指定循迹输入通道的原始电平。
 * @param  channel 通道编号，范围为 0 至 BOARD_TRACKING_INPUT_COUNT - 1。
 * @param  high 用于接收原始电平，true 表示高电平。
 * @return PLATFORM_OK 表示读取成功，其他 PLATFORM_* 表示参数或状态错误。
 * @note   本函数不执行滤波、消抖或有效电平反转。
 */
int board_tracking_read(uint8_t channel, bool *high)
{
  if (!s_initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return tracking_inputs_read_channel(&s_tracking_inputs,
                                      channel, high);
}

/**
 * @brief  获取已初始化的八路循迹输入组件。
 * @return 初始化成功后返回组件对象，否则返回 NULL。
 * @note   返回对象由 Board 层持有，调用者不得修改或释放。
 */
struct tracking_inputs *board_tracking_get(void)
{
  return s_initialized ? &s_tracking_inputs : NULL;
}
