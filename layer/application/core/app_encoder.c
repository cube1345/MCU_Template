/**
 * @file    app_encoder.c
 * @brief   左右编码器累计计数变化监测应用。
 * @author  weixin
 */
#include "app_encoder.h"

#include "board_drive.h"
#include "encoder.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 左右编码器累计计数的读取周期，单位为毫秒。 */
#define ENCODER_PERIOD_MS UINT32_C(20)

/**
 * @brief  启动并清零一个硬件编码器对象。
 * @param  encoder 已由Board层绑定的编码器基础对象。
 * @retval PLATFORM_OK表示启动和清零成功，其他PLATFORM_*表示失败。
 * @note   清零失败时会停止已经启动的编码器，避免保留半初始化状态。
 */
static int encoder_task_start(struct encoder *encoder)
{
  int result;

  result = encoder_start(encoder);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = encoder_reset(encoder);
  if (result != PLATFORM_OK) {
    (void)encoder_stop(encoder);
  }
  return result;
}

/**
 * @brief  尝试停止左右两个编码器对象。
 * @param  left_encoder 左编码器基础对象，可以为NULL。
 * @param  right_encoder 右编码器基础对象，可以为NULL。
 * @retval None。
 */
static void encoder_task_stop_pair(struct encoder *left_encoder,
                                   struct encoder *right_encoder)
{
  if ((right_encoder != NULL) &&
      encoder_is_initialized(right_encoder)) {
    (void)encoder_stop(right_encoder);
  }
  if ((left_encoder != NULL) &&
      encoder_is_initialized(left_encoder)) {
    (void)encoder_stop(left_encoder);
  }
}

/**
 * @brief  运行左右编码器变化监测任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占左右编码器，仅在任一路累计计数变化时输出。
 */
void encoder_task(void *argument)
{
  struct encoder *left_encoder;
  struct encoder *right_encoder;
  TickType_t last_wake_time;
  int32_t previous_left_count = 0;
  int32_t previous_right_count = 0;
  int32_t left_count;
  int32_t right_count;
  int result;

  (void)argument;

  left_encoder = board_encoder_left_get();
  right_encoder = board_encoder_right_get();
  if ((left_encoder == NULL) || (right_encoder == NULL)) {
    printf("[encoder] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  result = encoder_task_start(left_encoder);
  if (result == PLATFORM_OK) {
    result = encoder_task_start(right_encoder);
  }
  if (result != PLATFORM_OK) {
    encoder_task_stop_pair(left_encoder, right_encoder);
    printf("[encoder] start failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  printf("[encoder] left=TIM4 PD12/PD13 right=TIM3 PA6/PA7\r\n");
  last_wake_time = xTaskGetTickCount();
  for (;;) {
    result = encoder_read_count(left_encoder, &left_count);
    if (result == PLATFORM_OK) {
      result = encoder_read_count(right_encoder, &right_count);
    }
    if (result != PLATFORM_OK) {
      encoder_task_stop_pair(left_encoder, right_encoder);
      printf("[encoder] read failed: %d\r\n", result);
      vTaskDelete(NULL);
      return;
    }

    if ((left_count != previous_left_count) ||
        (right_count != previous_right_count)) {
      previous_left_count = left_count;
      previous_right_count = right_count;
      printf("[encoder] left=%ld right=%ld\r\n",
             (long)left_count, (long)right_count);
    }
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(ENCODER_PERIOD_MS));
  }
}
