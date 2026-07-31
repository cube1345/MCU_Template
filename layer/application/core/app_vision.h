/* SPDX-License-Identifier: MIT */
/**
 * @file    app_vision.h
 * @brief   Jetson视觉上位机USART3通信任务接口。
 * @author  weixin
 */
#ifndef APP_VISION_H
#define APP_VISION_H

#include <stdbool.h>
#include <stdint.h>

struct vision_ball_data {
  int16_t x;
  int8_t velocity;
  int8_t acceleration;
  uint32_t sequence;
  bool valid;
};

/**
 * @brief  运行Jetson视觉串口定长帧接收任务。
 * @param  argument FreeRTOS任务参数，当前未使用。
 * @retval None。
 * @note   本任务独占USART3，接收AA开头、FF结尾的6字节数据帧。
 */
void vision_task(void *argument);

/**
 * @brief  获取最近一次有效的Jetson钢球数据。
 * @param  data 用于接收数据的结构体指针。
 * @return PLATFORM_OK表示已有有效帧，尚未收到时返回状态错误。
 * @note   仅在任务上下文调用，不得从中断调用。
 */
int vision_get_ball_data(struct vision_ball_data *data);

/**
 * @brief  向Jetson发送4字节任务帧。
 * @param  data1 任务帧第1个数据字节。
 * @param  data2 任务帧第2个数据字节。
 * @return PLATFORM_OK表示发送成功，其他值表示UART或状态错误。
 * @note   仅在任务上下文调用；当前不会自动发送任何任务帧。
 */
int vision_send_task_frame(uint8_t data1, uint8_t data2);

#endif /* APP_VISION_H */
