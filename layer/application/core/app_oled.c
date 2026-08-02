/**
 * @file app_oled.c
 * @brief Mission status display task.
 */
#include "app_oled.h"

#include "app_mission.h"
#include "board_oled.h"
#include "display/display.h"
#include "graphics/mono_canvas.h"
#include "graphics/mono_text_ascii.h"
#include "graphics/mono_text_format.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app_log.h"

#define OLED_POLL_PERIOD_MS UINT32_C(100)
#define OLED_POWER_STABLE_DELAY_MS UINT32_C(50)

static int app_oled_draw_mission(
    struct mono_canvas *canvas,
    struct display *display,
    const struct mission_snapshot *snapshot)
{
  int result;

  result = mono_canvas_clear(canvas, false);
  if (result == PLATFORM_OK) {
    result = mono_text_printf(canvas, 0, 0, MONO_TEXT_ASCII_8X16,
                              "TASK %u/6",
                              (unsigned int)snapshot->selected_task);
  }
  if (result == PLATFORM_OK) {
    result = mono_text_ascii_draw_string(
        canvas, 0, 16, mission_task_name_get(snapshot->selected_task),
        MONO_TEXT_ASCII_8X16);
  }
  if (result == PLATFORM_OK) {
    result = mono_text_ascii_draw_string(
        canvas, 0, 32, mission_state_name_get(snapshot->state),
        MONO_TEXT_ASCII_8X16);
  }
  if (result == PLATFORM_OK) {
    if (snapshot->state == MISSION_STATE_ERROR) {
      result = mono_text_printf(canvas, 0, 48, MONO_TEXT_ASCII_8X16,
                                "ERR %d", snapshot->error_code);
    } else {
      result = mono_text_printf(canvas, 0, 48, MONO_TEXT_ASCII_8X16,
                                "TIME %lus",
                                (unsigned long)(snapshot->elapsed_ms /
                                                UINT32_C(1000)));
    }
  }
  return (result == PLATFORM_OK) ? display_flush(display) : result;
}

void oled_task(void *argument)
{
  struct mono_canvas *canvas;
  struct display *display;
  struct mission_snapshot snapshot = {0};
  TickType_t last_wake_time;
  uint32_t shown_revision = UINT32_MAX;
  uint32_t shown_seconds = UINT32_MAX;
  uint32_t current_seconds;
  int result;

  (void)argument;
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
    result = mission_snapshot_get(&snapshot);
    current_seconds = snapshot.elapsed_ms / UINT32_C(1000);
    if ((result == PLATFORM_OK) &&
        ((snapshot.revision != shown_revision) ||
         (current_seconds != shown_seconds))) {
      result = app_oled_draw_mission(canvas, display, &snapshot);
      if (result == PLATFORM_OK) {
        shown_revision = snapshot.revision;
        shown_seconds = current_seconds;
      }
    }
    if (result != PLATFORM_OK) {
      printf("[oled] update failed: %d\r\n", result);
    }
    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(OLED_POLL_PERIOD_MS));
  }
}
