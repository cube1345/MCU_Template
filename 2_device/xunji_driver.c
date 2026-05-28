#include "xunji_driver.h"
#include "../3_board/xunji_board.h"

static int32_t xunji_read_all_impl(xunji_t *me, uint8_t *xunji, uint8_t count)
{
    uint8_t i;
    uint8_t level;

    if ((me == 0) || (xunji == 0) || (count < XUNJI_COUNT) || (me->base.is_init == 0U)) {
        return -1;
    }

    for (i = 0U; i < XUNJI_COUNT; ++i) {
        level = me->cfg.pins[i]();
        xunji[i] = (me->cfg.active_high != 0U) ? level : (uint8_t)!level;
    }

    return 0;
}

static int32_t Xunji_Base_ReadAll(void *me, uint8_t *data, uint8_t count)
{
    return xunji_read_all_impl((xunji_t *)me, data, count);
}

static const xunji_base_ops_t g_xunji_base_ops = {
    .read_all = Xunji_Base_ReadAll,
};

static inline const xunji_base_ops_t *Xunji_GetBaseOps(void)
{
    return &g_xunji_base_ops;
}

void Xunji_InitDevice(xunji_t *me, const xunji_cfg_t *cfg)
{
    uint8_t i;

    if ((me == 0) || (cfg == 0)) {
        return;
    }

    for (i = 0U; i < XUNJI_COUNT; ++i) {
        if (cfg->pins[i] == 0) {
            me->base.is_init = 0U;
            return;
        }
    }

    me->ops = Xunji_GetBaseOps();
    me->cfg = *cfg;
    me->base.is_init = 1U;
}

int32_t Xunji_ReadAll(xunji_t *me, uint8_t *xunji, uint8_t count)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->read_all == 0)) {
        return -1;
    }
    return me->ops->read_all(me, xunji, count);
}

int Xunji_Driver_Bind(const Xunji_ReadPin_FP *pins, uint8_t count)
{
    xunji_cfg_t cfg;
    xunji_t *dev;
    uint8_t i;

    if ((pins == 0) || (count < XUNJI_COUNT)) {
        return -1;
    }

    for (i = 0U; i < XUNJI_COUNT; ++i) {
        cfg.pins[i] = pins[i];
    }
    cfg.active_high = 1U;

    dev = Xunji_Board_GetDefault();
    Xunji_InitDevice(dev, &cfg);
    return (dev->base.is_init != 0U) ? 0 : -1;
}

int Xunji_Get(uint8_t *xunji, uint8_t count)
{
    xunji_t *dev = Xunji_Board_GetDefault();
    return (int)Xunji_ReadAll(dev, xunji, count);
}

void Xunji_Init(void)
{
    /* Board-layer binding has been moved to Xunji_Board_Init(). */
}
