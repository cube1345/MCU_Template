#ifndef MYUSED_MYFACADES_XUNJI_FACADE_H
#define MYUSED_MYFACADES_XUNJI_FACADE_H

/**
 * @file xunji_facade.h
 * @brief 循迹模块兼容门面层接口。
 */

#include "../2_device/xunji_driver.h"

int Xunji_Get(uint8_t *xunji, uint8_t count);
void Xunji_Init(void);

#endif /* MYUSED_MYFACADES_XUNJI_FACADE_H */
