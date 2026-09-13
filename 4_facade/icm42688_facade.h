#ifndef MYUSED_MYFACADES_ICM42688_FACADE_H
#define MYUSED_MYFACADES_ICM42688_FACADE_H

/**
 * @file icm42688_facade.h
 * @brief ICM42688 兼容门面层接口。
 */

#include "../2_device/icm42688_driver.h"

void ICM42688_Facade_Init(void);
icm42688_t *ICM42688_Facade_GetDevice(void);
icm42688_status_t ICM42688_Facade_InitDevice(void);
icm42688_status_t ICM42688_Facade_ReadRaw(icm42688_raw_data_t *raw_data);
icm42688_status_t ICM42688_Facade_ReadScaled(icm42688_scaled_data_t *scaled_data);

#endif /* MYUSED_MYFACADES_ICM42688_FACADE_H */
