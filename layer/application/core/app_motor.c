/**
 * @file    app_motor.c
 * @brief   左右电机速度闭环控制应用。
 * @author  weixin
 */
#include "app_motor.h"

#include "app_serial_monitor.h"
#include "board_drive.h"
#include "motor.h"
#include "result.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app_log.h"

/* 应用层允许发布的单轮目标速度绝对值，单位为编码器计数每秒。 */
#define MOTOR_TARGET_SPEED_LIMIT INT32_C(200000)

/* 循迹任务尚未发布有效目标时保持停车。 */
static int32_t s_target_left_speed;
/* 与左轮目标组成同一原子快照的右轮目标速度。 */
static int32_t s_target_right_speed;

/**
 * @brief  原子发布左右轮目标速度。
 * @param  left_speed 左轮目标速度，单位为编码器计数每秒。
 * @param  right_speed 右轮目标速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示发布成功，超出安全范围时返回配置错误。
 * @note   临界区仅保护两个整数赋值，不执行硬件访问。
 */
int motor_target_speeds_set(int32_t left_speed,
                            int32_t right_speed)
{
  if ((left_speed < -MOTOR_TARGET_SPEED_LIMIT) ||
      (left_speed > MOTOR_TARGET_SPEED_LIMIT) ||
      (right_speed < -MOTOR_TARGET_SPEED_LIMIT) ||
      (right_speed > MOTOR_TARGET_SPEED_LIMIT)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  taskENTER_CRITICAL();
  s_target_left_speed = left_speed;
  s_target_right_speed = right_speed;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

/**
 * @brief  原子读取左右轮目标速度快照。
 * @param  left_speed 用于接收左轮目标速度的变量。
 * @param  right_speed 用于接收右轮目标速度的变量。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   临界区确保两个速度来自同一次发布。
 */
int motor_target_speeds_get(int32_t *left_speed,
                            int32_t *right_speed)
{
  if ((left_speed == NULL) || (right_speed == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  *left_speed = s_target_left_speed;
  *right_speed = s_target_right_speed;
  taskEXIT_CRITICAL();
  return PLATFORM_OK;
}

/**
 * @brief  在参数发生变化时将上位机 PI 设置应用到左右闭环轴。
 * @param  applied_kp 当前已经应用的比例增益。
 * @param  applied_ki 当前已经应用的积分增益。
 * @return PLATFORM_OK 表示无需更新或更新成功，其他 PLATFORM_* 表示失败。
 * @note   只能由电机控制任务调用，避免多个任务同时修改控制器状态。
 */
static int motor_task_apply_speed_gains(float *applied_kp,
                                        float *applied_ki)
{
  float requested_kp;
  float requested_ki;
  int result;

  result = serial_monitor_drive_speed_gains_get(&requested_kp,
                                                 &requested_ki);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((requested_kp == *applied_kp) &&
      (requested_ki == *applied_ki)) {
    return PLATFORM_OK;
  }

  result = board_drive_set_speed_gains(requested_kp,
                                       requested_ki);
  if (result == PLATFORM_OK) {
    *applied_kp = requested_kp;
    *applied_ki = requested_ki;
  }
  return result;
}

/**
 * @brief  在目标发生变化时将应用层速度设置应用到左右闭环轴。
 * @param  left_motor 左轮电机对象。
 * @param  right_motor 右轮电机对象。
 * @param  applied_left_speed 当前已应用的左轮目标速度。
 * @param  applied_right_speed 当前已应用的右轮目标速度。
 * @return PLATFORM_OK 表示无需更新或更新成功，其他 PLATFORM_* 表示失败。
 * @note   只能由电机控制任务调用，确保目标更新与闭环计算串行执行。
 */
static int motor_task_apply_target_speeds(
    struct motor *left_motor,
    struct motor *right_motor,
    int32_t *applied_left_speed,
    int32_t *applied_right_speed)
{
  int32_t requested_left_speed;
  int32_t requested_right_speed;
  int result;

  result = motor_target_speeds_get(
      &requested_left_speed, &requested_right_speed);
  if (result != PLATFORM_OK) {
    return result;
  }
  if ((requested_left_speed == *applied_left_speed) &&
      (requested_right_speed == *applied_right_speed)) {
    return PLATFORM_OK;
  }

  result = motor_set_speed(left_motor, requested_left_speed);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = motor_set_speed(right_motor, requested_right_speed);
  if (result != PLATFORM_OK) {
    return result;
  }

  *applied_left_speed = requested_left_speed;
  *applied_right_speed = requested_right_speed;
  return PLATFORM_OK;
}

/**
 * @brief  运行左右电机固定周期速度闭环任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占左右闭环电机轴及其编码器，禁止其他任务直接读取编码器。
 */
void motor_task(void *argument)
{
  struct motor *left_motor;
  struct motor *right_motor;
  TickType_t last_wake_time;
  float applied_kp = -1.0f;
  float applied_ki = -1.0f;
  int32_t applied_left_speed = INT32_MIN;
  int32_t applied_right_speed = INT32_MIN;
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
  int32_t left_speed;
  int32_t right_speed;
#endif
  int result;

  (void)argument;

  result = board_drive_init();
  if (result != PLATFORM_OK) {
    printf("[motor] drive init failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  left_motor = board_motor_left_get();
  right_motor = board_motor_right_get();
  if ((left_motor == NULL) || (right_motor == NULL)) {
    printf("[motor] motor get failed\r\n");
    vTaskDelete(NULL);
    return;
  }

  result = motor_task_apply_speed_gains(&applied_kp,
                                        &applied_ki);
  if (result != PLATFORM_OK) {
    (void)motor_coast(left_motor);
    (void)motor_coast(right_motor);
    printf("[motor] tuning apply failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  result = motor_task_apply_target_speeds(
      left_motor, right_motor,
      &applied_left_speed, &applied_right_speed);
  if (result != PLATFORM_OK) {
    (void)motor_coast(left_motor);
    (void)motor_coast(right_motor);
    printf("[motor] target apply failed: %d\r\n", result);
    vTaskDelete(NULL);
    return;
  }

  printf("[motor] closed loop target left=%ld right=%ld count/s\r\n",
         (long)applied_left_speed, (long)applied_right_speed);
  last_wake_time = xTaskGetTickCount();
  for (;;) {
    result = motor_task_apply_speed_gains(&applied_kp,
                                          &applied_ki);
    if (result == PLATFORM_OK) {
      result = motor_task_apply_target_speeds(
          left_motor, right_motor,
          &applied_left_speed, &applied_right_speed);
    }
    if (result == PLATFORM_OK) {
      result = board_drive_update();
    }
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
    if (result == PLATFORM_OK) {
      result = motor_get_speed(left_motor, &left_speed);
    }
    if (result == PLATFORM_OK) {
      result = motor_get_speed(right_motor, &right_speed);
    }
#endif
    if (result != PLATFORM_OK) {
      printf("[motor] control failed: %d\r\n", result);
      vTaskDelete(NULL);
      return;
    }
#if (SERIAL_MONITOR_DRIVE_DEBUG_ENABLE == 1)
    serial_monitor_drive_speeds_publish(left_speed, right_speed);
#endif

    vTaskDelayUntil(&last_wake_time,
                    pdMS_TO_TICKS(BOARD_DRIVE_UPDATE_PERIOD_MS));
  }
}
