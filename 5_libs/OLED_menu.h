#ifndef OLED_MENU_H
#define OLED_MENU_H

#include <stdint.h>
#include "../4_facade/oled_facade.h"

typedef struct {
    const char *Name;
    volatile float *Value;
    float Min;
    float Max;
} Menu_ParamItem_t;

void Menu_Key_Handler(void);
void Menu_Init(void);
void Menu_SetParamItems(const Menu_ParamItem_t *items, uint8_t count);

//使用方法
//第一步 OLED_Init();
//	    Key_Init();
//      Menu_Init();
//第二步 如需参数编辑，调用 Menu_SetParamItems() 注入参数表
//第三步 在循环里调用Menu_Key_Handler();

#endif
