/**
 * @file    app_oled.h
 * @brief   OLED 显示应用任务入口。
 * @author  weixin
 */
#ifndef APP_OLED_H
#define APP_OLED_H

/**
 * @brief  运行 OLED 递增计数显示任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占 OLED 画布和显示对象。
 */
void oled_task(void *argument);

#endif /* APP_OLED_H */
