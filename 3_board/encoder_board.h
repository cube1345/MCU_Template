#ifndef ENCODER_BOARD_H
#define ENCODER_BOARD_H

#include "../2_device/encoder_driver.h"

void Encoder_Board_Init(void);
encoder_t *Encoder_Board_GetA(void);
encoder_t *Encoder_Board_GetB(void);

#endif
