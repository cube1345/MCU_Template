#include "encoder_facade.h"
#include "encoder_board.h"

static int g_encoder_count_a;
static int g_encoder_count_b;

/*
 * @name: Encoder_Init
 * @note: Encoder业务层初始化入口，内部初始化板级Encoder对象
 * @return {void}
 * @author cube
 */
void Encoder_Init(void)
{
    Encoder_Board_Init();
    Encoder_ResetLegacyCounts();
}

/*
 * @name: Encoder_GetCountA/B
 * @note: 获取Encoder A/B当前计数值
 * @return {int32_t} 当前计数值
 * @author cube
 */
int32_t Encoder_GetCountA(void)
{
    return Encoder_GetCount(Encoder_Board_GetA());
}

int32_t Encoder_GetCountB(void)
{
    return Encoder_GetCount(Encoder_Board_GetB());
}

/*
 * @name: Encoder_ResetCountA/B
 * @note: 清零Encoder A/B当前计数值
 * @return {void}
 * @author cube
 */
void Encoder_ResetCountA(void)
{
    Encoder_ResetCount(Encoder_Board_GetA());
}

void Encoder_ResetCountB(void)
{
    Encoder_ResetCount(Encoder_Board_GetB());
}

/*
 * @name: Encoder_SyncLegacyCounts
 * @note: 将对象计数同步到旧业务兼容全局变量
 * @return {void}
 * @author cube
 */
void Encoder_SyncLegacyCounts(void)
{
    g_encoder_count_a = (int)Encoder_GetCountA();
    g_encoder_count_b = (int)Encoder_GetCountB();
}

/*
 * @name: Encoder_ResetLegacyCounts
 * @note: 清零Encoder对象计数并同步清零旧业务兼容全局变量
 * @return {void}
 * @author cube
 */
void Encoder_ResetLegacyCounts(void)
{
    Encoder_ResetCountA();
    Encoder_ResetCountB();
    g_encoder_count_a = 0;
    g_encoder_count_b = 0;
}

int32_t Encoder_GetLegacyCountA(void)
{
    return (int32_t)g_encoder_count_a;
}

int32_t Encoder_GetLegacyCountB(void)
{
    return (int32_t)g_encoder_count_b;
}
