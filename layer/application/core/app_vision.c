/* SPDX-License-Identifier: MIT */
/**
 * @file    app_vision.c
 * @brief   Jetson视觉串口USART3定长帧通信任务。
 * @author  weixin
 */
#include "app_vision.h"

#include "board_uart.h"
#include "platform/inferface/uart/uart_interrupt.h"
#include "result.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VISION_RX_QUEUE_LENGTH ((UBaseType_t)128U)
#define VISION_FRAME_SIZE UINT16_C(6)
#define VISION_FRAME_HEAD UINT8_C(0xAA)
#define VISION_FRAME_TAIL UINT8_C(0xFF)
#define VISION_X_MIN INT16_C(-320)
#define VISION_X_MAX INT16_C(320)
#define VISION_REARM_DELAY_MS UINT32_C(10)
#define VISION_TX_TIMEOUT_MS UINT32_C(100)

struct vision_rx_event {
  uint8_t byte;
  int8_t result;
};

static QueueHandle_t s_vision_rx_queue;
static struct uart *s_vision_uart;
static uint8_t s_vision_rx_byte;
static volatile bool s_vision_queue_overflow;
static volatile bool s_vision_rearm_required;
static struct vision_ball_data s_vision_ball;

static bool vision_flag_take(volatile bool *flag)
{
  bool value;

  taskENTER_CRITICAL();
  value = *flag;
  *flag = false;
  taskEXIT_CRITICAL();
  return value;
}

static void vision_rx_callback(struct uart *self,
                               uint8_t *data,
                               uint16_t size,
                               int result,
                               void *context)
{
  struct vision_rx_event event;
  BaseType_t task_woken = pdFALSE;
  int rearm_result;

  (void)context;
  event.byte = UINT8_C(0);
  event.result = (int8_t)result;
  if ((result == PLATFORM_OK) && (data != NULL) &&
      (size == UINT16_C(1))) {
    event.byte = data[0];
  } else if (result == PLATFORM_OK) {
    event.result = (int8_t)PLATFORM_ERR_IO;
  }

  rearm_result = uart_interrupt_receive(
      self, &s_vision_rx_byte, UINT16_C(1),
      vision_rx_callback, NULL);
  if (rearm_result != PLATFORM_OK) {
    s_vision_rearm_required = true;
  }
  if ((s_vision_rx_queue == NULL) ||
      (xQueueSendFromISR(s_vision_rx_queue, &event,
                         &task_woken) != pdPASS)) {
    s_vision_queue_overflow = true;
  }
  portYIELD_FROM_ISR(task_woken);
}

static void vision_receive_rearm(struct uart *uart)
{
  int result;

  do {
    result = uart_interrupt_receive(
        uart, &s_vision_rx_byte, UINT16_C(1),
        vision_rx_callback, NULL);
    if (result != PLATFORM_OK) {
      vTaskDelay(pdMS_TO_TICKS(VISION_REARM_DELAY_MS));
    }
  } while (result != PLATFORM_OK);
}

static void vision_parser_reset(uint8_t *frame, uint16_t *size)
{
  *size = UINT16_C(0);
  frame[0] = UINT8_C(0);
}

static bool vision_frame_decode(const uint8_t *frame)
{
  uint16_t x_raw;
  int16_t x;

  if ((frame == NULL) || (frame[0] != VISION_FRAME_HEAD) ||
      (frame[5] != VISION_FRAME_TAIL)) {
    return false;
  }
  x_raw = ((uint16_t)frame[1] << 8) | (uint16_t)frame[2];
  x = (int16_t)x_raw;
  if ((x < VISION_X_MIN) || (x > VISION_X_MAX)) {
    return false;
  }
  taskENTER_CRITICAL();
  s_vision_ball.x = x;
  s_vision_ball.velocity = (int8_t)frame[3];
  s_vision_ball.acceleration = (int8_t)frame[4];
  s_vision_ball.sequence++;
  s_vision_ball.valid = true;
  taskEXIT_CRITICAL();
  return true;
}

static void vision_parser_feed(uint8_t byte,
                               uint8_t *frame,
                               uint16_t *size)
{
  if (*size == UINT16_C(0)) {
    if (byte == VISION_FRAME_HEAD) {
      frame[0] = byte;
      *size = UINT16_C(1);
    }
    return;
  }
  frame[*size] = byte;
  (*size)++;
  if (*size < VISION_FRAME_SIZE) {
    return;
  }
  if (!vision_frame_decode(frame) && (byte == VISION_FRAME_HEAD)) {
    frame[0] = byte;
    *size = UINT16_C(1);
  } else {
    vision_parser_reset(frame, size);
  }
}

int vision_get_ball_data(struct vision_ball_data *data)
{
  if (data == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  taskENTER_CRITICAL();
  *data = s_vision_ball;
  taskEXIT_CRITICAL();
  return data->valid ? PLATFORM_OK : PLATFORM_ERR_INVALID_STATE;
}

int vision_send_task_frame(uint8_t data1, uint8_t data2)
{
  const uint8_t frame[] = {
    VISION_FRAME_HEAD, data1, data2, VISION_FRAME_TAIL
  };

  if (s_vision_uart == NULL) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  return uart_write(s_vision_uart, frame, (uint16_t)sizeof(frame),
                    VISION_TX_TIMEOUT_MS);
}

int vision_recording_start(void)
{
  return vision_send_task_frame(UINT8_C(0xBB), UINT8_C(0xBB));
}

int vision_recording_stop(void)
{
  return vision_send_task_frame(UINT8_C(0xCC), UINT8_C(0xCC));
}

void vision_task(void *argument)
{
  struct vision_rx_event event;
  uint8_t frame[VISION_FRAME_SIZE];
  uint16_t frame_size = UINT16_C(0);
  int result;

  (void)argument;
  result = board_uart3_init();
  s_vision_uart = board_uart3_get();
  if ((result != PLATFORM_OK) || (s_vision_uart == NULL) ||
      (s_vision_uart->interrupt_ops == NULL)) {
    s_vision_uart = NULL;
    vTaskDelete(NULL);
    return;
  }
  s_vision_rx_queue = xQueueCreate(
      VISION_RX_QUEUE_LENGTH,
      (UBaseType_t)sizeof(struct vision_rx_event));
  if (s_vision_rx_queue == NULL) {
    s_vision_uart = NULL;
    vTaskDelete(NULL);
    return;
  }
  s_vision_ball.valid = false;
  vision_receive_rearm(s_vision_uart);

  for (;;) {
    if (xQueueReceive(s_vision_rx_queue, &event,
                      portMAX_DELAY) != pdPASS) {
      continue;
    }
    if (vision_flag_take(&s_vision_rearm_required)) {
      vision_receive_rearm(s_vision_uart);
    }
    if (vision_flag_take(&s_vision_queue_overflow) ||
        (event.result != (int8_t)PLATFORM_OK)) {
      vision_parser_reset(frame, &frame_size);
      continue;
    }
    vision_parser_feed(event.byte, frame, &frame_size);
  }
}
