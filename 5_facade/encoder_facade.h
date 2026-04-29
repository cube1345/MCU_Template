#ifndef ENCODER_FACADE_H
#define ENCODER_FACADE_H

#include "stdint.h"
#include "../3_device/encoder_driver.h"

void Encoder_Init(void);
int32_t Encoder_GetCountA(void);
int32_t Encoder_GetCountB(void);
void Encoder_ResetCountA(void);
void Encoder_ResetCountB(void);
void Encoder_SyncLegacyCounts(void);
void Encoder_ResetLegacyCounts(void);
int32_t Encoder_GetLegacyCountA(void);
int32_t Encoder_GetLegacyCountB(void);

#endif
