/** @file mt6701_driver.h
 * @brief Instance-owned MT6701 SSI protocol and sampling state.
 */
#ifndef MT6701_DRIVER_H
#define MT6701_DRIVER_H
#include <stdbool.h>
#include <stdint.h>
#include "device_base.h"
#include "device_ops.h"

#define MT6701_ANGLE_BITS 14U
#define MT6701_ANGLE_MAX 16383U
#define MT6701_SSI_FRAME_BITS 24U
typedef enum { MT6701_AXIS_M1 = 0U, MT6701_AXIS_M2 = 1U } MT6701_Axis_t;

/** @brief Board callbacks own GPIO bindings, SSI edge delays and bus exclusion.
 * enter/leave must be supplied together. Caller serializes the whole object
 * operation across tasks; do not sample in an ISR.
 */
typedef struct {
    void *context;
    void (*write_clock)(void *context, uint8_t high);
    void (*write_select)(void *context, uint8_t axis, uint8_t high);
    uint8_t (*read_data)(void *context);
    void (*edge_delay)(void *context);
    uint32_t (*enter)(void *context);
    void (*leave)(void *context, uint32_t token);
} mt6701_bus_cfg_t;

/** @brief Caller-owned state for one chip, reset by InitDevice. */
typedef struct {
    int32_t last_angle_raw;
    uint8_t field_status;
    bool crc_valid;
    float speed_rad_s;
    float sample_angle_rad;
    bool sample_ready;
} mt6701_state_t;

typedef device_magnetic_encoder_ops_t mt6701_base_ops_t;
typedef struct mt6701_s {
    device_base_t base;
    const mt6701_base_ops_t *ops;
    mt6701_bus_cfg_t bus_cfg;
    mt6701_state_t *states;
    uint8_t count;
} mt6701_t;

/** @brief Bind 1..255 chips. states must be exclusive to this object and live
 * throughout its use. No allocation or fixed global device count is used.
 * Invalid configuration leaves is_init=0. Board prepares GPIO before binding.
 */
void MT6701_InitDevice(mt6701_t *me, const mt6701_bus_cfg_t *cfg,
                       mt6701_state_t *states, uint8_t count);
/** @brief Read a frame; -1 means uninitialized, invalid axis, or CRC failure. */
int32_t MT6701_ReadAngleRaw(mt6701_t *me, uint8_t axis);
/** @brief Read radians/degrees; on CRC failure return last valid angle. */
float MT6701_ReadAngleRad(mt6701_t *me, uint8_t axis);
float MT6701_ReadAngleDeg(mt6701_t *me, uint8_t axis);
/** @brief Read cached diagnostic fields without accessing the bus. */
uint8_t MT6701_GetFieldStatus(mt6701_t *me, uint8_t axis);
int32_t MT6701_GetLastAngleRaw(mt6701_t *me, uint8_t axis);
bool MT6701_IsLastCRCValid(mt6701_t *me, uint8_t axis);
float MT6701_GetSpeed(mt6701_t *me, uint8_t axis);
/** @brief Sample every millisecond in task/main context. Bad CRC resets speed
 * baseline; next valid sample establishes a fresh baseline.
 */
void MT6701_Service1ms(mt6701_t *me);
#endif
