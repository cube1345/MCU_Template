#include "icm42688_facade.h"

#include "../3_board/icm42688_board.h"

void ICM42688_Facade_Init(void)
{
    ICM42688_Board_Init();
}

icm42688_t *ICM42688_Facade_GetDevice(void)
{
    return ICM42688_Board_GetDevice();
}

icm42688_status_t ICM42688_Facade_InitDevice(void)
{
    icm42688_t *dev = ICM42688_Facade_GetDevice();

    if ((dev == (icm42688_t *)0) ||
        (dev->ops == (const icm42688_base_ops_t *)0) ||
        (dev->ops->init == (int32_t (*)(void *))0)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    return (icm42688_status_t)dev->ops->init(dev);
}

icm42688_status_t ICM42688_Facade_ReadRaw(icm42688_raw_data_t *raw_data)
{
    icm42688_t *dev = ICM42688_Facade_GetDevice();

    if ((raw_data == (icm42688_raw_data_t *)0) ||
        (dev == (icm42688_t *)0) ||
        (dev->ops == (const icm42688_base_ops_t *)0) ||
        (dev->ops->read_raw == (int32_t (*)(void *, void *))0)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    return (icm42688_status_t)dev->ops->read_raw(dev, raw_data);
}

icm42688_status_t ICM42688_Facade_ReadScaled(icm42688_scaled_data_t *scaled_data)
{
    icm42688_t *dev = ICM42688_Facade_GetDevice();

    if ((scaled_data == (icm42688_scaled_data_t *)0) ||
        (dev == (icm42688_t *)0) ||
        (dev->ops == (const icm42688_base_ops_t *)0) ||
        (dev->ops->read_scaled == (int32_t (*)(void *, void *))0)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    return (icm42688_status_t)dev->ops->read_scaled(dev, scaled_data);
}
