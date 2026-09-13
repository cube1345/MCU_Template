#include "xunji_facade.h"
#include "../3_board/xunji_board.h"

int Xunji_Get(uint8_t *xunji, uint8_t count)
{
    return (int)Xunji_ReadAll(Xunji_Board_GetDefault(), xunji, count);
}

void Xunji_Init(void)
{
    Xunji_Board_Init();
}
