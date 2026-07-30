/**
 * @file    app_encoder.h
 * @brief   左右编码器监测应用任务入口。
 * @author  weixin
 */
#ifndef APP_ENCODER_H
#define APP_ENCODER_H

/**
 * @brief  运行左右编码器变化监测任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占左右编码器，仅在任一路累计计数变化时输出。
 */
void encoder_task(void *argument);

#endif /* APP_ENCODER_H */
