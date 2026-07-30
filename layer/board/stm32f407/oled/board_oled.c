/* SPDX-License-Identifier: MIT */
/**
 * @file    board_oled.c
 * @brief   STM32F407的I2C1与SSD1306对象组合。
 * @author  weixin
 *
 * Board层只描述实际连线：SSD1306使用I2C1的PB6/PB7，
 * 使用7位地址0x3C。应用只获取display基类和单色画布。
 */
#include "board_oled.h"

#include "board_i2c.h"
#include "result.h"
#include "ssd1306/ssd1306.h"

#include <stdbool.h>
#include <stddef.h>

/* 板级SSD1306静态实例，生命周期覆盖整个程序运行期。 */
static struct ssd1306 s_oled;
/* 防止OLED被重复启动。 */
static bool s_initialized;

/**
 * @brief  绑定一次板级SSD1306对象。
 * @param  bus OLED连接的I2C1抽象总线对象。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 * @note   本函数只组合对象，不访问真实硬件。
 */
static int board_oled_bind_once(struct i2c *bus)
{
  if (s_oled.base.ops != NULL) {
    return PLATFORM_OK;
  }

  return ssd1306_init(&s_oled, "display0", bus,
                      SSD1306_DEFAULT_I2C_ADDRESS);
}

/**
 * @brief  初始化连接在I2C1上的SSD1306 OLED。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 * @note   初始化顺序为I2C1、SSD1306对象绑定、显示控制器启动。
 */
int board_oled_init(void)
{
  struct i2c *bus;
  struct display *display;
  int result;

  if (s_initialized) {
    return PLATFORM_OK;
  }

  result = board_i2c1_init();
  if (result != PLATFORM_OK) {
    return result;
  }
  bus = board_i2c1_get();
  if (bus == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = board_oled_bind_once(bus);
  if (result != PLATFORM_OK) {
    return result;
  }
  display = ssd1306_get_display(&s_oled);
  if (display == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  result = display_start(display);
  if (result == PLATFORM_OK) {
    s_initialized = true;
  }
  return result;
}

/**
 * @brief  获取板级OLED的显示设备基类。
 * @return 初始化成功后返回struct display *，否则返回NULL。
 */
struct display *board_oled_get(void)
{
  return s_initialized ? ssd1306_get_display(&s_oled) : NULL;
}

/**
 * @brief  获取板级OLED对应的单色画布。
 * @return 初始化成功后返回画布地址，否则返回NULL。
 */
struct mono_canvas *board_oled_canvas_get(void)
{
  return s_initialized ? ssd1306_get_canvas(&s_oled) : NULL;
}
