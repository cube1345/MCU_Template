/* SPDX-License-Identifier: MIT */
/**
 * @file    ssd1306.h
 * @brief   SSD1306 128x64 I2C显示控制器驱动对象。
 * @author  weixin
 */
#ifndef SSD1306_H
#define SSD1306_H

#include "display/display.h"
#include "graphics/mono_canvas.h"
#include "platform/inferface/i2c/i2c.h"

#include <stdint.h>

#define SSD1306_WIDTH UINT16_C(128)
#define SSD1306_HEIGHT UINT16_C(64)
#define SSD1306_PAGE_COUNT UINT16_C(8)
#define SSD1306_FRAMEBUFFER_SIZE UINT32_C(1024)
#define SSD1306_DEFAULT_I2C_ADDRESS UINT8_C(0x3C)

struct ssd1306 {
  /* 显示设备基类，必须位于首成员，供 container_of 安全下转型。 */
  struct display base;
  /* SSD1306 所挂载的I2C总线，由board层注入。 */
  struct i2c *bus;
  /* 供library绘图函数使用的单色画布。 */
  struct mono_canvas canvas;
  /* 7位I2C设备地址。 */
  uint8_t address;
  /* 128x64垂直分页显存，由具体对象持有。 */
  uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE];
};

/**
 * @brief  初始化SSD1306驱动对象并绑定display基类。
 * @param  self 调用者提供的SSD1306对象。
 * @param  name 板级逻辑名，字符串生命周期必须长于self。
 * @param  bus 已初始化或可用的I2C总线对象，生命周期必须长于self。
 * @param  address 未左移的7位I2C地址，通常为0x3C。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 * @note   self必须由调用者清零；本函数不访问硬件或分配堆内存。
 */
int ssd1306_init(struct ssd1306 *self,
                 const char *name,
                 struct i2c *bus,
                 uint8_t address);

/**
 * @brief  获取SSD1306对应的显示设备基类。
 * @param  self 已完成ssd1306_init()的驱动对象。
 * @return display基类地址；对象无效或未绑定时返回NULL。
 */
struct display *ssd1306_get_display(struct ssd1306 *self);

/**
 * @brief  获取SSD1306显存对应的单色画布。
 * @param  self 已完成ssd1306_init()的驱动对象。
 * @return 画布地址；对象无效或未绑定时返回NULL。
 * @note   画布和显示对象共享显存，调用者必须串行化绘图与刷新操作。
 */
struct mono_canvas *ssd1306_get_canvas(struct ssd1306 *self);

#endif /* SSD1306_H */
