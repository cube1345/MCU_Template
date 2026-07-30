/**
 * @file    led.h
 * @brief   LED 抽象基类、操作表和统一控制接口。
 * @author  weixin
 */
#ifndef LED_H
#define LED_H

#include "result.h"

#include <stdbool.h>
#include <stdint.h>

struct led;

/*
 * LED 抽象基类的虚函数表。
 *
 * on/off/toggle 是所有 LED 实现必须支持的行为。
 * set_brightness 作为少量固定操作之一保留在基础表中；不支持的实现用
 * NULL 绑定，统一包装函数返回 PLATFORM_ERR_UNSUPPORTED。
 */
struct led_ops {
    /* 打开 LED；必须实现。 */
    int (*on)(struct led *self);
    /* 关闭 LED；必须实现。 */
    int (*off)(struct led *self);
    /* 在打开和关闭状态之间翻转；必须实现。 */
    int (*toggle)(struct led *self);
    /* 设置亮度；不支持时绑定为 NULL。 */
    int (*set_brightness)(struct led *self, uint8_t percent);
};

struct led {
    /* 初始化时绑定，之后保持不变。 */
    const struct led_ops *ops;
    /* 便于调试识别对象，不作为查找键。 */
    const char *name;
    bool initialized;
};

/*
 * 调用者持有存储，LED 模块不使用堆内存。
 * 公共操作默认仅任务上下文调用，除非具体后端另行说明。
 * 具体实现必须把 struct led 作为首成员嵌入，并通过 container_of
 * 从基类指针恢复具体对象。
 */
/**
 * @brief 初始化 LED 抽象基类并绑定具体后端。
 * @param self 调用方持有的 LED 对象存储。
 * @param name 调试名称，调用期间必须保持有效。
 * @param ops 不可变操作表，on/off/toggle 均为必需操作。
 * @return 成功、参数错误或重复初始化。
 * @note 本函数不直接访问硬件；具体后端负责安全初始状态。
 */
int led_init(struct led *self, const char *name,
                            const struct led_ops *ops);

/**
 * @brief 打开 LED。
 * @param self 已初始化的 LED 基类句柄。
 * @return 成功、未初始化或后端错误。
 */
int led_on(struct led *self);

/**
 * @brief 关闭 LED。
 * @param self 已初始化的 LED 基类句柄。
 * @return 成功、未初始化或后端错误。
 */
int led_off(struct led *self);

/**
 * @brief 翻转 LED 当前开关状态。
 * @param self 已初始化的 LED 基类句柄。
 * @return 成功、未初始化或后端错误。
 */
int led_toggle(struct led *self);

/**
 * @brief 设置 LED 亮度。
 * @param self 已初始化的 LED 基类句柄。
 * @param percent 亮度百分比，范围为 0..100。
 * @return 成功、参数错误、未初始化或后端不支持。
 */
int led_set_brightness(struct led *self, uint8_t percent);

#endif /* LED_H */
