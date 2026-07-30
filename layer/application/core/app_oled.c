/**
 * @file    app_oled.c
 * @brief   OLED 递增计数显示应用。
 * @author  weixin
 */
#include "app_oled.h"

#include "board_oled.h"
#include "display/display.h"
#include "graphics/mono_canvas.h"
#include "graphics/mono_text_ascii.h"
#include "graphics/mono_text_format.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* OLED 计数内容的刷新周期，单位为毫秒。 */
#define OLED_PERIOD_MS UINT32_C(1000)
/* 等待 OLED 模块上电稳定的时间，单位为毫秒。 */
#define OLED_POWER_STABLE_DELAY_MS UINT32_C(50)
/* OLED 标题的纵坐标，单位为像素。 */
#define OLED_TITLE_Y INT16_C(0)
/* OLED 计数值的纵坐标，单位为像素。 */
#define OLED_COUNTER_Y INT16_C(16)

/**
 * @brief  将当前计数值绘制到 OLED 并刷新显示。
 * @param  canvas 已经初始化的 OLED 单色画布。
 * @param  display 已经启动的 OLED 显示设备。
 * @param  counter 需要显示的 32 位无符号计数值。
 * @return PLATFORM_OK 表示成功，其他 PLATFORM_* 表示失败。
 */
static int app_oled_draw_counter(struct mono_canvas *canvas,
                                 struct display *display,
                                 uint32_t counter)
{
  int result;

  /* 每轮先清除旧内容，避免位数变化后残留像素。 */
  result = mono_canvas_clear(canvas, false);
  if (result != PLATFORM_OK) {
    return result;
  }

  result = mono_text_ascii_draw_string(
      canvas, 0, OLED_TITLE_Y, "Count:", MONO_TEXT_ASCII_8X16);
  if (result != PLATFORM_OK) {
    return result;
  }

  result = mono_text_printf(canvas, 0, OLED_COUNTER_Y,
                            MONO_TEXT_ASCII_8X16,
                            "%lu", (unsigned long)counter);
  if (result != PLATFORM_OK) {
    return result;
  }

  /* 绘图函数只修改内存画布，刷新后内容才会传输到 OLED。 */
  return display_flush(display);
}

/**
 * @brief  运行 OLED 递增计数显示任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占 OLED 画布和显示对象，并使用阻塞 I2C 刷新。
 */
void oled_task(void *argument)
{
  struct mono_canvas *canvas;
  struct display *display;
  TickType_t last_wake_time;
  uint32_t counter = 0U;
  int result;

  (void)argument;

  /*
   * 调度器启动后先等待 OLED 电源和 I2C 引脚电平稳定，避免模块仍处于
   * 上电复位阶段时访问 I2C1，导致外设检测到总线持续忙。
   */
  vTaskDelay(pdMS_TO_TICKS(OLED_POWER_STABLE_DELAY_MS));

  result = board_oled_init();
  if (result != PLATFORM_OK) {
    printf("[oled] init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  canvas = board_oled_canvas_get();
  display = board_oled_get();
  if ((canvas == NULL) || (display == NULL)) {
    printf("[oled] get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  last_wake_time = xTaskGetTickCount();
  for (;;) {
    result = app_oled_draw_counter(canvas, display, counter);
    if (result != PLATFORM_OK) {
      printf("[oled] update failed: %d\r\n", result);
    }

    /* 达到 32 位最大值后回到 0，保持计数行为明确。 */
    counter = (counter == UINT32_MAX) ? 0U : counter + 1U;
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(OLED_PERIOD_MS));
  }
}
