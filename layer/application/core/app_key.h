/**
 * @file    app_key.h
 * @brief   用户按键应用任务入口。
 * @author  weixin
 */
#ifndef APP_KEY_H
#define APP_KEY_H

/**
 * @brief  运行用户按键事件轮询任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务负责初始化并独占用户按键及其事件状态机。
 */
void key_task(void *argument);

#endif /* APP_KEY_H */
