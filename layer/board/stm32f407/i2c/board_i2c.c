/* SPDX-License-Identifier: MIT */
/**
 * @file    board_i2c.c
 * @brief   STM32F407开发板I2C1板级资源配置。
 * @author  weixin
 *
 * 这里描述板卡实际连线和使用的实现方式。应用层只获取 struct i2c，
 * 不需要知道 I2C1 最终由 HAL 外设还是 GPIO 位操作实现。
 */
#include "board_i2c.h"

#include "result.h"
#include "stm32f4.h"
#include "stm32f4_i2c.h"

#include <stddef.h>

#define BOARD_I2C1_BACKEND_HARDWARE INT32_C(0)
#define BOARD_I2C1_BACKEND_SOFTWARE INT32_C(1)
#define BOARD_I2C1_BACKEND BOARD_I2C1_BACKEND_HARDWARE

static const struct i2c_bus_config g_i2c1_config = {
  .scl_pin = STM32F4_PIN_PB6,
  .sda_pin = STM32F4_PIN_PB7,
  .speed_hz = UINT32_C(100000),
  .timeout_ms = UINT32_C(1000),
};

static struct i2c *g_i2c1;
static bool g_i2c1_initialized;

/**
 * @brief  初始化 STM32F407 板级 I2C1 总线。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU 初始化、后端注册或总线获取失败。
 * @note   默认使用 PB6/PB7，后端由 BOARD_I2C1_BACKEND 宏选择。
 */
int board_i2c1_init(void)
{
  int result;

  if (g_i2c1_initialized) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  if (result != PLATFORM_OK) {
    return result;
  }
#if BOARD_I2C1_BACKEND == BOARD_I2C1_BACKEND_HARDWARE
  result = stm32f4_i2c_register(STM32F4_I2C1, &g_i2c1_config);
#elif BOARD_I2C1_BACKEND == BOARD_I2C1_BACKEND_SOFTWARE
  result = stm32f4_i2c_sw_register(STM32F4_I2C1, &g_i2c1_config);
#else
#error "Unsupported BOARD_I2C1_BACKEND"
#endif
  if (result != PLATFORM_OK) {
    return result;
  }
  g_i2c1 = i2c_bus_get_by_id(STM32F4_I2C1);
  g_i2c1_initialized = (g_i2c1 != NULL);
  return g_i2c1_initialized ? PLATFORM_OK : PLATFORM_ERR_IO;
}

/**
 * @brief  获取板级 I2C1 抽象总线对象。
 * @return 初始化成功后返回 struct i2c *，否则返回 NULL。
 * @note   应用和驱动应通过 platform I2C 接口使用该对象。
 */
struct i2c *board_i2c1_get(void)
{
  return g_i2c1_initialized ? g_i2c1 : NULL;
}
