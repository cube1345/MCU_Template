/** @file mt6701_driver.c
 * @brief SSI frame decode and independent sampling state for each instance.
 */
#include "mt6701_driver.h"
#include <stddef.h>
#include <string.h>

static bool MT6701_Valid(const mt6701_t *me, uint8_t axis)
{
    return me && me->base.is_init && me->states && axis < me->count;
}

/** @brief CRC-6 x^6+x+1 of the 18 angle/status bits, MSB first. */
static uint8_t MT6701_CRC(uint32_t data)
{
    uint8_t crc = 0U;
    for (int i = 17; i >= 0; --i) {
        uint8_t bit = (uint8_t)((data >> i) & 1U);
        crc <<= 1;
        if ((crc >> 6) ^ bit) { crc ^= 0x43U; }
        crc &= 0x3FU;
    }
    return crc;
}

int32_t MT6701_ReadAngleRaw(mt6701_t *me, uint8_t axis)
{
    if (!MT6701_Valid(me, axis)) { return -1; }
    const mt6701_bus_cfg_t *bus = &me->bus_cfg;
    uint32_t token = bus->enter ? bus->enter(bus->context) : 0U;
    uint32_t frame = 0U;
    bus->write_select(bus->context, axis, 0U);
    bus->edge_delay(bus->context);
    for (uint32_t bit = 0U; bit < MT6701_SSI_FRAME_BITS; ++bit) {
        bus->write_clock(bus->context, 1U);
        bus->edge_delay(bus->context);
        frame = (frame << 1) | (bus->read_data(bus->context) ? 1U : 0U);
        bus->write_clock(bus->context, 0U);
        bus->edge_delay(bus->context);
    }
    bus->write_select(bus->context, axis, 1U);
    bus->edge_delay(bus->context);
    if (bus->leave) { bus->leave(bus->context, token); }
    mt6701_state_t *state = &me->states[axis];
    state->field_status = (uint8_t)((frame >> 6) & 0xFU);
    state->crc_valid = MT6701_CRC(frame >> 6) == (frame & 0x3FU);
    if (!state->crc_valid) { return -1; }
    state->last_angle_raw = (int32_t)((frame >> 10) & MT6701_ANGLE_MAX);
    return state->last_angle_raw;
}

static float MT6701_Radians(int32_t raw)
{
    return raw < 0 ? 0.0f : (float)raw * (6.283185307179586f / 16384.0f);
}

float MT6701_ReadAngleRad(mt6701_t *me, uint8_t axis)
{
    if (!MT6701_Valid(me, axis)) { return 0.0f; }
    MT6701_ReadAngleRaw(me, axis);
    return MT6701_Radians(me->states[axis].last_angle_raw);
}

float MT6701_ReadAngleDeg(mt6701_t *me, uint8_t axis)
{
    return MT6701_ReadAngleRad(me, axis) * 57.29577951308232f;
}

uint8_t MT6701_GetFieldStatus(mt6701_t *me, uint8_t axis)
{
    return MT6701_Valid(me, axis) ? me->states[axis].field_status : 0U;
}

int32_t MT6701_GetLastAngleRaw(mt6701_t *me, uint8_t axis)
{
    return MT6701_Valid(me, axis) ? me->states[axis].last_angle_raw : -1;
}

bool MT6701_IsLastCRCValid(mt6701_t *me, uint8_t axis)
{
    return MT6701_Valid(me, axis) && me->states[axis].crc_valid;
}

float MT6701_GetSpeed(mt6701_t *me, uint8_t axis)
{
    return MT6701_Valid(me, axis) ? me->states[axis].speed_rad_s : 0.0f;
}

void MT6701_Service1ms(mt6701_t *me)
{
    if (!MT6701_Valid(me, 0U)) { return; }
    for (uint8_t axis = 0U; axis < me->count; ++axis) {
        mt6701_state_t *state = &me->states[axis];
        int32_t raw = MT6701_ReadAngleRaw(me, axis);
        if (raw < 0) {
            state->sample_ready = false;
            state->speed_rad_s = 0.0f;
            continue;
        }
        float angle = MT6701_Radians(raw);
        if (state->sample_ready) {
            float diff = angle - state->sample_angle_rad;
            if (diff > 3.141592653589793f) { diff -= 6.283185307179586f; }
            if (diff < -3.141592653589793f) { diff += 6.283185307179586f; }
            state->speed_rad_s = diff * 1000.0f;
        }
        state->sample_angle_rad = angle;
        state->sample_ready = true;
    }
}

static int32_t MT6701_BaseRaw(void *me, uint8_t axis) { return MT6701_ReadAngleRaw(me, axis); }
static float MT6701_BaseRad(void *me, uint8_t axis) { return MT6701_ReadAngleRad(me, axis); }
static float MT6701_BaseDeg(void *me, uint8_t axis) { return MT6701_ReadAngleDeg(me, axis); }
static uint8_t MT6701_BaseField(void *me, uint8_t axis) { return MT6701_GetFieldStatus(me, axis); }
static int32_t MT6701_BaseLast(void *me, uint8_t axis) { return MT6701_GetLastAngleRaw(me, axis); }
static uint8_t MT6701_BaseCRC(void *me, uint8_t axis) { return MT6701_IsLastCRCValid(me, axis); }
static float MT6701_BaseSpeed(void *me, uint8_t axis) { return MT6701_GetSpeed(me, axis); }
static void MT6701_BaseService(void *me) { MT6701_Service1ms(me); }

static const mt6701_base_ops_t mt6701_ops = {
    .read_angle_raw = MT6701_BaseRaw, .read_angle_rad = MT6701_BaseRad,
    .read_angle_deg = MT6701_BaseDeg, .get_field_status = MT6701_BaseField,
    .get_last_angle_raw = MT6701_BaseLast, .is_crc_valid = MT6701_BaseCRC,
    .get_speed = MT6701_BaseSpeed, .service = MT6701_BaseService,
};

void MT6701_InitDevice(mt6701_t *me, const mt6701_bus_cfg_t *cfg,
                       mt6701_state_t *states, uint8_t count)
{
    if (!me) { return; }
    mt6701_bus_cfg_t copy = {0};
    if (cfg) { copy = *cfg; }
    memset(me, 0, sizeof(*me));
    me->ops = &mt6701_ops;
    if (!states || !count || !copy.write_clock || !copy.write_select ||
        !copy.read_data || !copy.edge_delay || (!!copy.enter != !!copy.leave)) { return; }
    me->bus_cfg = copy;
    me->states = states;
    me->count = count;
    memset(states, 0, sizeof(*states) * count);
    uint32_t token = copy.enter ? copy.enter(copy.context) : 0U;
    for (uint8_t axis = 0U; axis < count; ++axis) {
        states[axis].last_angle_raw = -1;
        copy.write_select(copy.context, axis, 1U);
    }
    copy.write_clock(copy.context, 0U);
    copy.edge_delay(copy.context);
    if (copy.leave) { copy.leave(copy.context, token); }
    me->base.is_init = 1U;
}
