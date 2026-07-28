/**
 * @file motor_board.h
 * @brief Default-board binding for the DC motor Device.
 * @author cube
 * @date 2026-07-29
 * @note Updated: 2026-07-29
 */

#ifndef MOTOR_BOARD_H
#define MOTOR_BOARD_H

#include "../2_device/motor_driver.h"

/** @brief Initialize the default board motor and its two PWM channels. */
void Motor_Board_Init(void);

/** @brief Return the default board motor instance. */
motor_t *Motor_Board_GetDefault(void);

#endif /* MOTOR_BOARD_H */
