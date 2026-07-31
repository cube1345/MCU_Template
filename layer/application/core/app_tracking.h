/**
 * @file    app_tracking.h
 * @brief   八路循迹差速控制任务入口。
 * @author  weixin
 */
#ifndef APP_TRACKING_H
#define APP_TRACKING_H

#include <stdbool.h>
#include <stdint.h>

#define APP_TRACKING_WEIGHT_COUNT UINT8_C(8)

struct tracking_status {
  bool sample_valid;
  bool stop_marker_raw_detected;
  bool stop_marker_detected;
  bool stop_marker_clear_confirmed;
  uint8_t longest_adjacent_black;
  uint8_t stop_marker_confirm_count;
  uint8_t stop_marker_clear_count;
};

int tracking_enabled_set(bool enabled);
int tracking_enabled_get(bool *enabled);
int tracking_base_speeds_set(int32_t left_speed, int32_t right_speed);
int tracking_manual_speeds_set(int32_t left_speed, int32_t right_speed);
int tracking_weights_set(
    const int8_t weights[APP_TRACKING_WEIGHT_COUNT]);
int tracking_status_get(struct tracking_status *status);

/**
 * @brief  周期采样八路循迹输入并发布左右轮目标速度。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @retval None。
 * @note   任务不访问电机硬件，左右闭环仍由电机任务独占执行。
 */
void tracking_task(void *argument);

#endif /* APP_TRACKING_H */
