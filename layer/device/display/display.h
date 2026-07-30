/* SPDX-License-Identifier: MIT */
/**
 * @file    display.h
 * @brief   显示设备基础对象和统一刷新接口。
 * @author  weixin
 *
 * display 只描述所有显示控制器共有的生命周期和刷新能力。
 * 具体控制器协议、通信总线和显存布局由 driver 层实现。
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

struct display;

struct display_info {
  uint16_t width;
  uint16_t height;
};

struct display_area {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
};

struct display_ops {
  int (*start)(struct display *self);
  int (*stop)(struct display *self);
  int (*flush)(struct display *self);
  int (*flush_area)(struct display *self,
                    const struct display_area *area);
  int (*set_power)(struct display *self, bool enabled);
  int (*set_contrast)(struct display *self, uint8_t contrast);
};

struct display {
  const struct display_ops *ops;
  const char *name;
  const char *model;
  struct display_info info;
  bool initialized;
};

/**
 * @brief  绑定显示设备基础对象和具体驱动操作表。
 * @param  self 调用者提供的显示设备基础对象。
 * @param  name 板级逻辑名，字符串生命周期必须长于 self。
 * @param  model 显示控制器型号，字符串生命周期必须长于 self。
 * @param  info 显示区域宽度和高度，函数会复制其内容。
 * @param  ops 具体驱动操作表，start 和 flush 为必选能力。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   self必须由调用者清零；本函数不访问硬件或分配堆内存。
 */
int display_bind(struct display *self,
                 const char *name,
                 const char *model,
                 const struct display_info *info,
                 const struct display_ops *ops);

/**
 * @brief  启动显示控制器。
 * @param  self 已完成 display_bind() 的显示设备对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   具体驱动可能执行阻塞总线事务，只能在任务上下文调用。
 */
int display_start(struct display *self);

/**
 * @brief  停止显示控制器。
 * @param  self 已启动的显示设备对象。
 * @return PLATFORM_OK 表示成功；不支持时返回 PLATFORM_ERR_UNSUPPORTED。
 * @note   具体驱动可能执行阻塞总线事务，只能在任务上下文调用。
 */
int display_stop(struct display *self);

/**
 * @brief  将完整显存刷新到显示控制器。
 * @param  self 已启动的显示设备对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 * @note   本函数可能长时间阻塞，调用者必须串行化同一显示对象的访问。
 */
int display_flush(struct display *self);

/**
 * @brief  将指定矩形覆盖范围刷新到显示控制器。
 * @param  self 已启动的显示设备对象。
 * @param  area 待刷新区域，必须完全位于显示范围内。
 * @return PLATFORM_OK 表示成功；不支持时返回 PLATFORM_ERR_UNSUPPORTED。
 * @note   本函数可能阻塞，且不保证由多个任务并发调用时安全。
 */
int display_flush_area(struct display *self,
                       const struct display_area *area);

/**
 * @brief  设置显示面板电源状态。
 * @param  self 已启动的显示设备对象。
 * @param  enabled true 表示开启显示，false 表示关闭显示。
 * @return PLATFORM_OK 表示成功；不支持时返回 PLATFORM_ERR_UNSUPPORTED。
 * @note   本函数可能执行阻塞总线事务，不能在中断中调用。
 */
int display_set_power(struct display *self, bool enabled);

/**
 * @brief  设置显示对比度。
 * @param  self 已启动的显示设备对象。
 * @param  contrast 对比度值，范围由具体控制器解释。
 * @return PLATFORM_OK 表示成功；不支持时返回 PLATFORM_ERR_UNSUPPORTED。
 * @note   本函数可能执行阻塞总线事务，不能在中断中调用。
 */
int display_set_contrast(struct display *self, uint8_t contrast);

/**
 * @brief  获取显示区域信息。
 * @param  self 显示设备对象。
 * @return 显示信息地址；对象无效时返回 NULL。
 * @note   返回值指向对象内部只读数据，不得释放或修改。
 */
const struct display_info *display_get_info(const struct display *self);

/**
 * @brief  获取显示设备逻辑名。
 * @param  self 显示设备对象。
 * @return 逻辑名；对象无效时返回 "unknown"。
 */
const char *display_get_name(const struct display *self);

/**
 * @brief  获取显示控制器型号名。
 * @param  self 显示设备对象。
 * @return 型号名；对象无效时返回 "unknown"。
 */
const char *display_get_model(const struct display *self);

/**
 * @brief  查询显示设备是否已经启动。
 * @param  self 显示设备对象。
 * @return true 表示已启动；false 表示未启动或对象无效。
 */
bool display_is_initialized(const struct display *self);

#endif /* DISPLAY_H */
