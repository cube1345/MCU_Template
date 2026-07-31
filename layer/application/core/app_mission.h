/* SPDX-License-Identifier: MIT */
#ifndef APP_MISSION_H
#define APP_MISSION_H

#include <stdint.h>

enum mission_state {
  MISSION_STATE_SELECTING = 0,
  MISSION_STATE_RUNNING,
  MISSION_STATE_COMPLETE,
  MISSION_STATE_TIMEOUT,
  MISSION_STATE_ERROR
};

enum mission_key_event {
  MISSION_KEY_SHORT_RELEASE = 0,
  MISSION_KEY_LONG_RELEASE
};

struct mission_snapshot {
  uint8_t selected_task;
  enum mission_state state;
  uint8_t substep;
  uint32_t elapsed_ms;
  int error_code;
  uint32_t revision;
};

int mission_init(void);
int mission_key_event_post(enum mission_key_event event);
int mission_snapshot_get(struct mission_snapshot *snapshot);
const char *mission_task_name_get(uint8_t task_number);
const char *mission_state_name_get(enum mission_state state);
void mission_task(void *argument);

#endif /* APP_MISSION_H */
