#include "encoder_driver.h"

static int32_t encoder_get_count_impl(encoder_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return 0;
    }
    return me->count;
}

static void encoder_reset_count_impl(encoder_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }
    me->count = 0;
}

static void encoder_update_from_ab_impl(encoder_t *me,
                                        uint8_t changed_a,
                                        uint8_t changed_b,
                                        uint8_t level_a,
                                        uint8_t level_b)
{
    int8_t delta = 0;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    if (changed_a != 0U) {
        delta = (level_b == 0U) ? 1 : -1;
    } else if (changed_b != 0U) {
        delta = (level_a == 0U) ? -1 : 1;
    }

    me->count += (int32_t)(delta * me->cfg.dir);
}

static int32_t Encoder_Base_GetCount(void *me)
{
    return encoder_get_count_impl((encoder_t *)me);
}

static void Encoder_Base_ResetCount(void *me)
{
    encoder_reset_count_impl((encoder_t *)me);
}

static void Encoder_Base_UpdateFromAB(void *me,
                                      uint8_t changed_a,
                                      uint8_t changed_b,
                                      uint8_t level_a,
                                      uint8_t level_b)
{
    encoder_update_from_ab_impl((encoder_t *)me, changed_a, changed_b, level_a, level_b);
}

int32_t Encoder_GetCount(encoder_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->get_count == 0)) {
        return 0;
    }
    return me->ops->get_count(me);
}

void Encoder_ResetCount(encoder_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->reset_count == 0)) {
        return;
    }
    me->ops->reset_count(me);
}

void Encoder_UpdateFromAB(encoder_t *me,
                          uint8_t changed_a,
                          uint8_t changed_b,
                          uint8_t level_a,
                          uint8_t level_b)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->update_from_ab == 0)) {
        return;
    }
    me->ops->update_from_ab(me, changed_a, changed_b, level_a, level_b);
}

/*
 * @name: g_encoder_base_ops
 * @note: Encoder基础操作函数集合
 */
static const encoder_base_ops_t g_encoder_base_ops = {
    .get_count = Encoder_Base_GetCount,
    .reset_count = Encoder_Base_ResetCount,
    .update_from_ab = Encoder_Base_UpdateFromAB,
};


/*
 * @name: Encoder_GetBaseOps
 * @note: 获取Encoder基础操作函数集合的指针
 */
static inline const encoder_base_ops_t *Encoder_GetBaseOps(void)
{
    return &g_encoder_base_ops;
}

/*
 * @name: Encoder_InitDevice
 * @note: 初始化Encoder设备对象
 * @param {encoder_t *} me - Encoder设备实例指针
 * @param {const encoder_cfg_t *} cfg - Encoder配置信息指针
 * @return {void}
 * @author cube
 */
void Encoder_InitDevice(encoder_t *me, const encoder_cfg_t *cfg)
{
    if ((me == 0) || (cfg == 0)) {
        return;
    }

    me->ops = Encoder_GetBaseOps();
    me->cfg = *cfg;
    if (me->cfg.dir == 0) {
        me->cfg.dir = 1;
    }
    me->count = 0;
    me->base.is_init = 1U;
}
