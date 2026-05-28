/**
 * @file    encoder_board.c
 * @brief   Encoder 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 实例化 Encoder A/B 设备对象，绑定 A/B 相 GPIO 配置
 * - 在 GROUP1_IRQHandler 中处理 GPIO 中断，调用 Encoder_UpdateFromAB() 更新计数
 * - 所有 GPIO 操作通过 HAL 层（HAL_GPIO_* / HAL_GPIO_ClearInterruptStatus），
 *   不直接调用 DL_GPIO_*
 */

#include "encoder_board.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "ti_msp_dl_config.h"

static encoder_t g_encoder_a;
static encoder_t g_encoder_b;

/*
 * @name: Encoder_Board_GetA
 * @note: 获取板级Encoder A对象指针
 * @return {encoder_t *} Encoder A对象指针
 * @author cube
 */
encoder_t *Encoder_Board_GetA(void)
{
    return &g_encoder_a;
}

/*
 * @name: Encoder_Board_GetB
 * @note: 获取板级Encoder B对象指针
 * @return {encoder_t *} Encoder B对象指针
 * @author cube
 */
encoder_t *Encoder_Board_GetB(void)
{
    return &g_encoder_b;
}

/*
 * @name: Encoder_Board_Init
 * @note: 初始化板级Encoder对象，填充A/B相GPIO配置并打开中断
 * @return {void}
 * @author cube
 */
void Encoder_Board_Init(void)
{
    // const encoder_cfg_t encoder_a_cfg = {
    //     .phase_a = {
    //         .port = ENCODERA_PORT,
    //         .pin = ENCODERA_E1A_PIN,
    //     },
    //     .phase_b = {
    //         .port = ENCODERA_PORT,
    //         .pin = ENCODERA_E1B_PIN,
    //     },
    //     .dir = 1,
    // };
    // const encoder_cfg_t encoder_b_cfg = {
    //     .phase_a = {
    //         .port = ENCODERB_PORT,
    //         .pin = ENCODERB_E2A_PIN,
    //     },
    //     .phase_b = {
    //         .port = ENCODERB_PORT,
    //         .pin = ENCODERB_E2B_PIN,
    //     },
    //     .dir = -1,
    // };

    // Encoder_InitDevice(&g_encoder_a, &encoder_a_cfg);
    // Encoder_InitDevice(&g_encoder_b, &encoder_b_cfg);

    // NVIC_ClearPendingIRQ(ENCODERA_INT_IRQN);
    // NVIC_ClearPendingIRQ(ENCODERB_INT_IRQN);
    // NVIC_EnableIRQ(ENCODERA_INT_IRQN);
    // NVIC_EnableIRQ(ENCODERB_INT_IRQN);
}

/*
 * @name: GROUP1_IRQHandler
 * @note: GPIO组1中断服务函数，读取Encoder A/B相中断状态并更新计数
 * @return {void}
 * @author cube
 */
void GROUP1_IRQHandler(void)
{
    // uint32_t gpio_interrupt_a;
    // uint32_t gpio_interrupt_b;
    // uint8_t changed_a;
    // uint8_t changed_b;

    // gpio_interrupt_a = HAL_GPIO_GetEnabledInterruptStatus(ENCODERA_PORT,
    //                                                       ENCODERA_E1A_PIN | ENCODERA_E1B_PIN);
    // gpio_interrupt_b = HAL_GPIO_GetEnabledInterruptStatus(ENCODERB_PORT,
    //                                                       ENCODERB_E2A_PIN | ENCODERB_E2B_PIN);

    // changed_a = (gpio_interrupt_a & ENCODERA_E1A_PIN) == ENCODERA_E1A_PIN;
    // changed_b = (gpio_interrupt_a & ENCODERA_E1B_PIN) == ENCODERA_E1B_PIN;
    // Encoder_UpdateFromAB(&g_encoder_a,
    //                      changed_a,
    //                      changed_b,
    //                      HAL_GPIO_ReadPin(ENCODERA_PORT, ENCODERA_E1A_PIN),
    //                      HAL_GPIO_ReadPin(ENCODERA_PORT, ENCODERA_E1B_PIN));

    // changed_a = (gpio_interrupt_b & ENCODERB_E2A_PIN) == ENCODERB_E2A_PIN;
    // changed_b = (gpio_interrupt_b & ENCODERB_E2B_PIN) == ENCODERB_E2B_PIN;
    // Encoder_UpdateFromAB(&g_encoder_b,
    //                      changed_a,
    //                      changed_b,
    //                      HAL_GPIO_ReadPin(ENCODERB_PORT, ENCODERB_E2A_PIN),
    //                      HAL_GPIO_ReadPin(ENCODERB_PORT, ENCODERB_E2B_PIN));

    // HAL_GPIO_ClearInterruptStatus(ENCODERA_PORT, ENCODERA_E1A_PIN | ENCODERA_E1B_PIN);
    // HAL_GPIO_ClearInterruptStatus(ENCODERB_PORT, ENCODERB_E2A_PIN | ENCODERB_E2B_PIN);
}
