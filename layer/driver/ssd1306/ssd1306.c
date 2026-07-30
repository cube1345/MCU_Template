/* SPDX-License-Identifier: MIT */
/**
 * @file    ssd1306.c
 * @brief   SSD1306 I2C初始化、显存刷新和显示控制实现。
 * @author  weixin
 */
#include "ssd1306/ssd1306.h"

#include "container_of.h"
#include "platform/inferface/i2c/i2c_blocking.h"

#include <stddef.h>

#define SSD1306_CONTROL_COMMAND UINT16_C(0x00)
#define SSD1306_CONTROL_DATA UINT16_C(0x40)
#define SSD1306_COMMAND_DISPLAY_OFF UINT8_C(0xAE)
#define SSD1306_COMMAND_DISPLAY_ON UINT8_C(0xAF)
#define SSD1306_COMMAND_SET_CONTRAST UINT8_C(0x81)
#define SSD1306_COMMAND_SET_COLUMN_RANGE UINT8_C(0x21)
#define SSD1306_COMMAND_SET_PAGE_RANGE UINT8_C(0x22)
#define SSD1306_MIN_7BIT_ADDRESS UINT8_C(0x08)
#define SSD1306_MAX_7BIT_ADDRESS UINT8_C(0x77)

/* SSD1306 128x64、内部电荷泵和水平寻址模式初始化命令。 */
static const uint8_t s_init_commands[] = {
  UINT8_C(0xAE), UINT8_C(0xD5), UINT8_C(0x80),
  UINT8_C(0xA8), UINT8_C(0x3F), UINT8_C(0xD3),
  UINT8_C(0x00), UINT8_C(0x40), UINT8_C(0x8D),
  UINT8_C(0x14), UINT8_C(0x20), UINT8_C(0x00),
  UINT8_C(0xA1), UINT8_C(0xC8), UINT8_C(0xDA),
  UINT8_C(0x12), UINT8_C(0x81), UINT8_C(0xCF),
  UINT8_C(0xD9), UINT8_C(0xF1), UINT8_C(0xDB),
  UINT8_C(0x30), UINT8_C(0xA4), UINT8_C(0xA6),
};

/**
 * @brief  通过I2C发送一组命令或显存数据。
 * @param  self SSD1306驱动对象。
 * @param  control SSD1306控制字节，0x00为命令，0x40为数据。
 * @param  data 待发送数据。
 * @param  size 数据长度。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_write(struct ssd1306 *self,
                         uint16_t control,
                         const uint8_t *data,
                         uint16_t size)
{
  struct i2c_mem_address address;

  if ((self == NULL) || (self->bus == NULL) ||
      (data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  address.device_addr = self->address;
  address.memory_addr = control;
  address.memory_addr_size = PLATFORM_I2C_MEM_ADDR_SIZE_8BIT;
  return i2c_mem_write(self->bus, &address, data, size);
}

/**
 * @brief  发送单个SSD1306命令。
 * @param  self SSD1306驱动对象。
 * @param  command 命令字节。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_write_command(struct ssd1306 *self,
                                 uint8_t command)
{
  return ssd1306_write(self, SSD1306_CONTROL_COMMAND,
                       &command, UINT16_C(1));
}

/**
 * @brief  发送一组SSD1306命令。
 * @param  self SSD1306驱动对象。
 * @param  commands 命令数组。
 * @param  size 命令字节数。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_write_commands(struct ssd1306 *self,
                                  const uint8_t *commands,
                                  uint16_t size)
{
  return ssd1306_write(self, SSD1306_CONTROL_COMMAND,
                       commands, size);
}

/**
 * @brief  刷新完整SSD1306显存。
 * @param  self SSD1306驱动对象。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_flush_full(struct ssd1306 *self)
{
  static const uint8_t commands[] = {
    SSD1306_COMMAND_SET_COLUMN_RANGE, UINT8_C(0), UINT8_C(127),
    SSD1306_COMMAND_SET_PAGE_RANGE, UINT8_C(0), UINT8_C(7),
  };
  int result;

  result = ssd1306_write_commands(
      self, commands, (uint16_t)sizeof(commands));
  if (result != PLATFORM_OK) {
    return result;
  }

  return ssd1306_write(self, SSD1306_CONTROL_DATA,
                       self->framebuffer,
                       (uint16_t)sizeof(self->framebuffer));
}

/**
 * @brief  刷新一个页面内指定列范围。
 * @param  self SSD1306驱动对象。
 * @param  page 页面编号，范围0到7。
 * @param  start_x 起始列。
 * @param  end_x 结束列。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_flush_page(struct ssd1306 *self,
                              uint8_t page,
                              uint8_t start_x,
                              uint8_t end_x)
{
  uint8_t commands[] = {
    SSD1306_COMMAND_SET_COLUMN_RANGE, start_x, end_x,
    SSD1306_COMMAND_SET_PAGE_RANGE, page, page,
  };
  uint16_t data_size;
  uint32_t data_offset;
  int result;

  result = ssd1306_write_commands(
      self, commands, (uint16_t)sizeof(commands));
  if (result != PLATFORM_OK) {
    return result;
  }

  data_size = (uint16_t)end_x - start_x + UINT16_C(1);
  data_offset = (uint32_t)page * SSD1306_WIDTH + start_x;
  return ssd1306_write(self, SSD1306_CONTROL_DATA,
                       &self->framebuffer[data_offset], data_size);
}

/**
 * @brief  启动SSD1306并清空屏幕。
 * @param  base display基类对象。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_start(struct display *base)
{
  struct ssd1306 *self;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);

  result = mono_canvas_clear(&self->canvas, false);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = ssd1306_write_commands(
      self, s_init_commands, (uint16_t)sizeof(s_init_commands));
  if (result != PLATFORM_OK) {
    return result;
  }

  result = ssd1306_flush_full(self);
  if (result != PLATFORM_OK) {
    (void)ssd1306_write_command(self,
                                SSD1306_COMMAND_DISPLAY_OFF);
    return result;
  }

  return ssd1306_write_command(self,
                               SSD1306_COMMAND_DISPLAY_ON);
}

/**
 * @brief  关闭SSD1306显示输出。
 * @param  base display基类对象。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_stop(struct display *base)
{
  struct ssd1306 *self;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);
  return ssd1306_write_command(self, SSD1306_COMMAND_DISPLAY_OFF);
}

/**
 * @brief  刷新完整SSD1306显存。
 * @param  base display基类对象。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_flush(struct display *base)
{
  struct ssd1306 *self;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);
  return ssd1306_flush_full(self);
}

/**
 * @brief  刷新指定矩形覆盖的SSD1306页面。
 * @param  base display基类对象。
 * @param  area 待刷新区域。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_flush_area(
    struct display *base,
    const struct display_area *area)
{
  struct ssd1306 *self;
  uint16_t start_page;
  uint16_t end_page;
  uint16_t page;
  uint8_t start_x;
  uint8_t end_x;
  int result;

  if ((base == NULL) || (area == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);
  start_page = area->y / UINT16_C(8);
  end_page = (uint16_t)((area->y + area->height - 1U) /
                        UINT16_C(8));
  start_x = (uint8_t)area->x;
  end_x = (uint8_t)(area->x + area->width - 1U);

  for (page = start_page; page <= end_page; page++) {
    result = ssd1306_flush_page(self, (uint8_t)page,
                                start_x, end_x);
    if (result != PLATFORM_OK) {
      return result;
    }
  }
  return PLATFORM_OK;
}

/**
 * @brief  设置SSD1306显示电源状态。
 * @param  base display基类对象。
 * @param  enabled true表示开启，false表示关闭。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_set_power(struct display *base, bool enabled)
{
  struct ssd1306 *self;
  uint8_t command;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);
  command = enabled ? SSD1306_COMMAND_DISPLAY_ON :
                      SSD1306_COMMAND_DISPLAY_OFF;
  return ssd1306_write_command(self, command);
}

/**
 * @brief  设置SSD1306对比度。
 * @param  base display基类对象。
 * @param  contrast 对比度值。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
static int ssd1306_set_contrast(struct display *base,
                                uint8_t contrast)
{
  struct ssd1306 *self;
  uint8_t commands[] = {
    SSD1306_COMMAND_SET_CONTRAST, contrast,
  };

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct ssd1306, base);
  return ssd1306_write_commands(
      self, commands, (uint16_t)sizeof(commands));
}

/* SSD1306绑定到display抽象层的操作表。 */
static const struct display_ops s_ssd1306_ops = {
  .start = ssd1306_start,
  .stop = ssd1306_stop,
  .flush = ssd1306_flush,
  .flush_area = ssd1306_flush_area,
  .set_power = ssd1306_set_power,
  .set_contrast = ssd1306_set_contrast,
};

/**
 * @brief  初始化SSD1306驱动对象并绑定display基类。
 * @param  self 调用者提供的SSD1306对象。
 * @param  name 板级逻辑名。
 * @param  bus I2C总线对象。
 * @param  address 未左移的7位I2C地址。
 * @return PLATFORM_OK表示成功；其他PLATFORM_*表示失败。
 */
int ssd1306_init(struct ssd1306 *self,
                 const char *name,
                 struct i2c *bus,
                 uint8_t address)
{
  static const struct display_info info = {
    .width = SSD1306_WIDTH,
    .height = SSD1306_HEIGHT,
  };
  int result;

  if ((self == NULL) || (name == NULL) || (bus == NULL) ||
      (address < SSD1306_MIN_7BIT_ADDRESS) ||
      (address > SSD1306_MAX_7BIT_ADDRESS)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->base.ops != NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->bus = bus;
  self->address = address;
  result = mono_canvas_init(&self->canvas, self->framebuffer,
                            sizeof(self->framebuffer),
                            SSD1306_WIDTH, SSD1306_HEIGHT);
  if (result != PLATFORM_OK) {
    return result;
  }

  return display_bind(&self->base, name, "SSD1306",
                      &info, &s_ssd1306_ops);
}

/**
 * @brief  获取SSD1306对应的显示设备基类。
 * @param  self SSD1306驱动对象。
 * @return display基类地址；对象无效或未绑定时返回NULL。
 */
struct display *ssd1306_get_display(struct ssd1306 *self)
{
  if ((self == NULL) || (self->base.ops == NULL)) {
    return NULL;
  }
  return &self->base;
}

/**
 * @brief  获取SSD1306显存对应的单色画布。
 * @param  self SSD1306驱动对象。
 * @return 画布地址；对象无效或未绑定时返回NULL。
 */
struct mono_canvas *ssd1306_get_canvas(struct ssd1306 *self)
{
  if ((self == NULL) || (self->base.ops == NULL)) {
    return NULL;
  }
  return &self->canvas;
}
