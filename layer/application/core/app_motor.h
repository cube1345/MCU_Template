/**
 * @file    app_motor.h
 * @brief   左右电机速度闭环应用任务入口。
 * @author  weixin
 */
#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#include <stdint.h>

/**
 * @brief  原子发布左右轮目标速度。
 * @param  left_speed 左轮目标速度，单位为编码器计数每秒。
 * @param  right_speed 右轮目标速度，单位为编码器计数每秒。
 * @return PLATFORM_OK 表示发布成功，超出安全范围时返回配置错误。
 * @note   仅可在任务上下文调用；本函数不直接访问电机硬件。
 */
int motor_target_speeds_set(int32_t left_speed,
                            int32_t right_speed);

/**
 * @brief  原子读取左右轮目标速度快照。
 * @param  left_speed 用于接收左轮目标速度的变量。
 * @param  right_speed 用于接收右轮目标速度的变量。
 * @return PLATFORM_OK 表示读取成功，参数为空时返回参数错误。
 * @note   仅可在任务上下文调用；左右轮数值来自同一次快照。
 */
int motor_target_speeds_get(int32_t *left_speed,
                            int32_t *right_speed);

/**
 * @brief  运行左右电机固定周期速度闭环任务。
 * @param  argument FreeRTOS 任务参数，当前未使用。
 * @return 无返回值，任务启动后持续运行。
 * @note   本任务独占左右闭环电机轴以及对应编码器。
 */
void motor_task(void *argument);

#endif /* APP_MOTOR_H */
