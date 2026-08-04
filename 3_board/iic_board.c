/**
 * @file    iic_board.c
 * @brief   当前板级 IIC 实例、IRQ 与 SysConfig 宏绑定。
 * @author  cube
 * @date    2026-08-03
 *
 * 本文件可以随 SysConfig 配置裁剪：未生成的 IIC 实例不会被引用，
 * 通用 HAL 仍然可以独立编译。
 */

#include "iic_board.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "hal_iic.h"
#include "my_system_lib.h"
#include "ti_msp_dl_config.h"

static bool g_iic_board_initialized;

static I2C_Regs *iic_board_get_instance(void *context)
{
    if (context != (void *)0) {
        return (I2C_Regs *)context;
    }

    return IIC_Board_GetDefault();
}

static device_iic_status_t iic_board_convert_status(hal_iic_status_t status)
{
    if (status == HAL_IIC_STATUS_OK) {
        return DEVICE_IIC_STATUS_OK;
    }

    if (status == HAL_IIC_STATUS_ERROR_PARAM) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    return DEVICE_IIC_STATUS_ERROR_TRANSFER;
}

/**
 * @brief  注册并初始化一个已由 SysConfig 配置完成的 IIC 控制器。
 * @param  iic_instance IIC 控制器寄存器基址。
 * @param  bus_speed_hz IIC 总线速率，单位为 Hz。
 * @retval true 初始化成功；false 初始化失败。
 */
static bool iic_board_init_instance(I2C_Regs *iic_instance, uint32_t bus_speed_hz)
{
    return (hal_iic_init(iic_instance, bus_speed_hz, My_GetTimeUs) == HAL_IIC_STATUS_OK);
}

/**
 * @brief  根据当前传输模式配置一个 IIC 中断通道。
 * @param  interrupt_number IIC 控制器对应的 NVIC 中断号。
 */
static void iic_board_configure_irq(IRQn_Type interrupt_number)
{
#if (HAL_IIC_TRANSFER_MODE == HAL_IIC_TRANSFER_MODE_IRQ)
    NVIC_SetPriority(interrupt_number, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_ClearPendingIRQ(interrupt_number);
    NVIC_EnableIRQ(interrupt_number);
#else
    NVIC_DisableIRQ(interrupt_number);
#endif
}

static device_iic_status_t iic_board_init(void *context)
{
    IIC_Board_Init();

    if (iic_board_get_instance(context) == (I2C_Regs *)0) {
        return DEVICE_IIC_STATUS_ERROR_PARAM;
    }

    return DEVICE_IIC_STATUS_OK;
}

static device_iic_status_t iic_board_probe(void *context, uint8_t address)
{
    return iic_board_convert_status(
        hal_iic_check_device(iic_board_get_instance(context), address));
}

static device_iic_status_t iic_board_write(void *context, uint8_t address,
    const uint8_t *data, uint16_t length)
{
    return iic_board_convert_status(
        hal_iic_write_bytes(iic_board_get_instance(context), address, data, length));
}

static device_iic_status_t iic_board_read(void *context, uint8_t address,
    uint8_t *data, uint16_t length)
{
    return iic_board_convert_status(
        hal_iic_read_bytes(iic_board_get_instance(context), address, data, length));
}

static device_iic_status_t iic_board_write_reg8(void *context, uint8_t address,
    uint8_t reg_addr, uint8_t reg_data)
{
    return iic_board_convert_status(
        hal_iic_write_reg8(iic_board_get_instance(context), address, reg_addr, reg_data));
}

static device_iic_status_t iic_board_read_reg8(void *context, uint8_t address,
    uint8_t reg_addr, uint8_t *reg_data)
{
    return iic_board_convert_status(
        hal_iic_read_reg8(iic_board_get_instance(context), address, reg_addr, reg_data));
}

static device_iic_status_t iic_board_write_reg8s(void *context, uint8_t address,
    uint8_t reg_addr, const uint8_t *data, uint16_t length)
{
    return iic_board_convert_status(
        hal_iic_write_reg8s(iic_board_get_instance(context), address, reg_addr, data, length));
}

static device_iic_status_t iic_board_read_reg8s(void *context, uint8_t address,
    uint8_t reg_addr, uint8_t *data, uint16_t length)
{
    return iic_board_convert_status(
        hal_iic_read_reg8s(iic_board_get_instance(context), address, reg_addr, data, length));
}

const device_iic_ops_t g_iic_board_default_ops = {
    .init = iic_board_init,
    .probe = iic_board_probe,
    .write = iic_board_write,
    .read = iic_board_read,
    .write_reg8 = iic_board_write_reg8,
    .read_reg8 = iic_board_read_reg8,
    .write_reg8s = iic_board_write_reg8s,
    .read_reg8s = iic_board_read_reg8s,
};

void IIC_Board_Init(void)
{
    bool initialized = false;

    if (g_iic_board_initialized == true) {
        return;
    }

#if defined(I2C_0_INST)
    if (iic_board_init_instance(I2C_0_INST, I2C_0_BUS_SPEED_HZ) == true) {
        initialized = true;
#if defined(I2C_0_INST_INT_IRQN)
        iic_board_configure_irq(I2C_0_INST_INT_IRQN);
#endif
    }
#endif

#if defined(I2C_1_INST)
    if (iic_board_init_instance(I2C_1_INST, I2C_1_BUS_SPEED_HZ) == true) {
        initialized = true;
#if defined(I2C_1_INST_INT_IRQN)
        iic_board_configure_irq(I2C_1_INST_INT_IRQN);
#endif
    }
#endif

    g_iic_board_initialized = initialized;
}

I2C_Regs *IIC_Board_GetDefault(void)
{
#if defined(I2C_0_INST)
    return I2C_0_INST;
#elif defined(I2C_1_INST)
    return I2C_1_INST;
#else
    return (I2C_Regs *)0;
#endif
}

#if defined(I2C_0_INST)
void I2C_0_INST_IRQHandler(void)
{
    hal_iic_irq_handler(I2C_0_INST);
}
#endif

#if defined(I2C_1_INST)
void I2C_1_INST_IRQHandler(void)
{
    hal_iic_irq_handler(I2C_1_INST);
}
#endif
