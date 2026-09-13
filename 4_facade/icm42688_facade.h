#ifndef MYUSED_MYFACADES_ICM42688_FACADE_H
#define MYUSED_MYFACADES_ICM42688_FACADE_H

/**
 * @file icm42688_facade.h
 * @brief ICM42688 兼容门面层接口。
 */

#include "../2_device/icm42688_driver.h"

void ICM42688_Facade_Init(void);
icm42688_t *ICM42688_Facade_GetDevice(void);
/** @brief Initialize the selected device using its bound configuration.
 * @param dev Bound device instance.
 * @return ICM42688 status; readiness is set only on successful initialization.
 */
icm42688_status_t ICM42688_Facade_InitDevice(icm42688_t *dev);
/** @brief Read a raw sample from the selected initialized device.
 * @param dev Initialized device instance.
 * @param raw_data Output sample.
 * @return ICM42688 status.
 */
icm42688_status_t ICM42688_Facade_ReadRaw(icm42688_t *dev, icm42688_raw_data_t *raw_data);
/** @brief Read physical units using the selected device's current range.
 * @param dev Initialized device instance.
 * @param scaled_data Output sample in g, dps, rad/s and degrees Celsius.
 * @return ICM42688 status.
 */
icm42688_status_t ICM42688_Facade_ReadScaled(icm42688_t *dev, icm42688_scaled_data_t *scaled_data);

#endif /* MYUSED_MYFACADES_ICM42688_FACADE_H */
