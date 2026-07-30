/* SPDX-License-Identifier: MIT */
/**
 * @file    display.c
 * @brief   显示设备基础对象的生命周期和操作分派。
 * @author  weixin
 */
#include "display/display.h"

#include <stddef.h>

/**
 * @brief  检查显示设备是否已经启动。
 * @param  self 显示设备对象。
 * @return PLATFORM_OK 表示可用；其他 PLATFORM_* 表示状态错误。
 */
static int display_ready_check(const struct display *self)
{
  if ((self == NULL) || (self->ops == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  return PLATFORM_OK;
}

/**
 * @brief  检查刷新区域是否位于显示范围内。
 * @param  self 显示设备对象。
 * @param  area 待检查区域。
 * @return true 表示区域有效；false 表示区域无效。
 */
static bool display_area_is_valid(
    const struct display *self,
    const struct display_area *area)
{
  uint32_t end_x;
  uint32_t end_y;

  if ((self == NULL) || (area == NULL) ||
      (area->width == 0U) || (area->height == 0U)) {
    return false;
  }

  end_x = (uint32_t)area->x + area->width;
  end_y = (uint32_t)area->y + area->height;
  return (end_x <= self->info.width) &&
         (end_y <= self->info.height);
}

/**
 * @brief  绑定显示设备基础对象和具体驱动操作表。
 * @param  self 调用者提供的显示设备基础对象。
 * @param  name 板级逻辑名。
 * @param  model 显示控制器型号。
 * @param  info 显示区域信息。
 * @param  ops 具体驱动操作表。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_bind(struct display *self,
                 const char *name,
                 const char *model,
                 const struct display_info *info,
                 const struct display_ops *ops)
{
  if ((self == NULL) || (name == NULL) || (model == NULL) ||
      (info == NULL) || (ops == NULL) || (ops->start == NULL) ||
      (ops->flush == NULL) || (info->width == 0U) ||
      (info->height == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if ((self->ops != NULL) || self->initialized) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->ops = ops;
  self->name = name;
  self->model = model;
  self->info = *info;
  self->initialized = false;
  return PLATFORM_OK;
}

/**
 * @brief  启动显示控制器。
 * @param  self 已完成绑定的显示设备对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_start(struct display *self)
{
  int result;

  if ((self == NULL) || (self->ops == NULL) ||
      (self->ops->start == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->initialized) {
    return PLATFORM_OK;
  }

  result = self->ops->start(self);
  if (result == PLATFORM_OK) {
    self->initialized = true;
  }
  return result;
}

/**
 * @brief  停止显示控制器。
 * @param  self 已启动的显示设备对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_stop(struct display *self)
{
  int result;

  result = display_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (self->ops->stop == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  result = self->ops->stop(self);
  if (result == PLATFORM_OK) {
    self->initialized = false;
  }
  return result;
}

/**
 * @brief  刷新完整显示区域。
 * @param  self 已启动的显示设备对象。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_flush(struct display *self)
{
  int result;

  result = display_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }

  return self->ops->flush(self);
}

/**
 * @brief  刷新指定矩形覆盖范围。
 * @param  self 已启动的显示设备对象。
 * @param  area 待刷新区域。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_flush_area(struct display *self,
                       const struct display_area *area)
{
  int result;

  result = display_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (!display_area_is_valid(self, area)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (self->ops->flush_area == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->ops->flush_area(self, area);
}

/**
 * @brief  设置显示面板电源状态。
 * @param  self 已启动的显示设备对象。
 * @param  enabled true 表示开启，false 表示关闭。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_set_power(struct display *self, bool enabled)
{
  int result;

  result = display_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (self->ops->set_power == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->ops->set_power(self, enabled);
}

/**
 * @brief  设置显示对比度。
 * @param  self 已启动的显示设备对象。
 * @param  contrast 对比度值。
 * @return PLATFORM_OK 表示成功；其他 PLATFORM_* 表示失败。
 */
int display_set_contrast(struct display *self, uint8_t contrast)
{
  int result;

  result = display_ready_check(self);
  if (result != PLATFORM_OK) {
    return result;
  }
  if (self->ops->set_contrast == NULL) {
    return PLATFORM_ERR_UNSUPPORTED;
  }

  return self->ops->set_contrast(self, contrast);
}

/**
 * @brief  获取显示区域信息。
 * @param  self 显示设备对象。
 * @return 显示信息地址；对象无效时返回 NULL。
 */
const struct display_info *display_get_info(const struct display *self)
{
  if ((self == NULL) || (self->ops == NULL)) {
    return NULL;
  }
  return &self->info;
}

/**
 * @brief  获取显示设备逻辑名。
 * @param  self 显示设备对象。
 * @return 逻辑名；对象无效时返回 "unknown"。
 */
const char *display_get_name(const struct display *self)
{
  if ((self == NULL) || (self->name == NULL)) {
    return "unknown";
  }
  return self->name;
}

/**
 * @brief  获取显示控制器型号名。
 * @param  self 显示设备对象。
 * @return 型号名；对象无效时返回 "unknown"。
 */
const char *display_get_model(const struct display *self)
{
  if ((self == NULL) || (self->model == NULL)) {
    return "unknown";
  }
  return self->model;
}

/**
 * @brief  查询显示设备是否已经启动。
 * @param  self 显示设备对象。
 * @return true 表示已启动；false 表示未启动或对象无效。
 */
bool display_is_initialized(const struct display *self)
{
  return (self != NULL) && self->initialized;
}
