/**
 * @file    app_led.h
 * @brief   LED 应用任务入口。
 * @author  weixin
 */
#ifndef APP_LED_H
#define APP_LED_H

/**
 * @brief  运行板载蓝色 LED 周期翻转任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务负责初始化并独占板载蓝色 LED 对象。
 */
void led_task(void *argument);

#endif /* APP_LED_H */
