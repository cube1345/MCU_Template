/**
 * @file    oled_board.c
 * @brief   OLED 板级绑定层
 * @author  cube
 * @date    2026-04
 *
 * 职责：
 * - 提供 OLED 软件 IIC 的 SCL/SDA GPIO 写函数
 * - 将软件 IIC 实现为公共 device_iic_t transport
 * - 提供 OLED_Board_Init() 统一初始化入口
 *
 * OLED 使用软件 IIC（非硬件 IIC 外设），其传输实现仅属于 Board 层。
 * GPIO 操作通过 HAL 层 HAL_GPIO_WritePin()，不直接调用 DL_GPIO_*。
 */

#include "oled_board.h"
#include "hal_gpio.h"
#include "ti_msp_dl_config.h"

#define OLED_IIC_ADDRESS              (0x3CU)

/** @brief 软件 I2C SCL 线写函数，通过 HAL 层操作 GPIO */
static void OLED_SCL_Write(uint8_t state)
{
    HAL_GPIO_WritePin(OLED_PORT, OLED_SCL0_PIN, state);
}

/** @brief 软件 I2C SDA 线写函数，通过 HAL 层操作 GPIO */
static void OLED_SDA_Write(uint8_t state)
{
    HAL_GPIO_WritePin(OLED_PORT, OLED_SDA0_PIN, state);
}

/** @brief 在软件 IIC 总线上产生起始条件。 */
static void oled_board_iic_start(void)
{
    OLED_SDA_Write(1U);
    OLED_SCL_Write(1U);
    OLED_SDA_Write(0U);
    OLED_SCL_Write(0U);
}

/** @brief 在软件 IIC 总线上产生停止条件。 */
static void oled_board_iic_stop(void)
{
    OLED_SDA_Write(0U);
    OLED_SCL_Write(1U);
    OLED_SDA_Write(1U);
}

/**
 * @brief  在软件 IIC 总线上发送 1 字节；当前 OLED 接口不采样 ACK。
 * @param  data 待发送数据。
 */
static void oled_board_iic_write_byte(uint8_t data)
{
    uint8_t bit_index;

    for (bit_index = 0U; bit_index < 8U; bit_index++) {
        OLED_SDA_Write((uint8_t)((data & (0x80U >> bit_index)) != 0U));
        OLED_SCL_Write(1U);
        OLED_SCL_Write(0U);
    }

    OLED_SCL_Write(1U);
    OLED_SCL_Write(0U);
}

/** @brief 初始化 Board 软件 IIC transport，空 context 表示默认 OLED 连线。 */
static device_iic_status_t oled_board_iic_init(void *context)
{
    (void)context;

    OLED_SCL_Write(1U);
    OLED_SDA_Write(1U);
    return DEVICE_IIC_STATUS_OK;
}

/**
 * @brief  向软件 IIC 从机写入一个连续字节包。
 * @note   当前回调不具备 SDA 读取能力，无法报告 NACK；行为与原 OLED 驱动保持一致。
 */
static device_iic_status_t oled_board_iic_write(void *context, uint8_t address,
    const uint8_t *data, uint16_t length)
{
    uint16_t data_index;

    (void)context;

    if ((data == (const uint8_t *)0) || (length == 0U) || (address > 0x7FU)) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    oled_board_iic_start();
    oled_board_iic_write_byte((uint8_t)(address << 1U));
    for (data_index = 0U; data_index < length; data_index++) {
        oled_board_iic_write_byte(data[data_index]);
    }
    oled_board_iic_stop();

    return DEVICE_IIC_STATUS_OK;
}

static const device_iic_ops_t g_oled_board_iic_ops = {
    .init = oled_board_iic_init,
    .write = oled_board_iic_write,
};

static const device_iic_t g_oled_board_iic = {
    .ops = &g_oled_board_iic_ops,
    .context = (void *)0,
    .address = OLED_IIC_ADDRESS,
};

/** @brief 将默认 OLED 绑定到 Board 软件 IIC endpoint。 */
void OLED_Driver_Init(void)
{
    OLED_BindDefaultIic(&g_oled_board_iic);
}

/** @brief OLED 板级初始化：绑定 IIC transport 并初始化 OLED 控制器。 */
void OLED_Board_Init(void)
{
    OLED_Driver_Init();
    OLED_Init();
}
