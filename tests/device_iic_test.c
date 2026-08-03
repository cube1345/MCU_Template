/**
 * @file device_iic_test.c
 * @brief Host test for the board-injected Device IIC endpoint contract.
 * @author cube
 * @date 2026-08-03
 * @note Updated: 2026-08-03
 */

#include <assert.h>
#include <stdint.h>

#include "device_iic.h"

typedef struct
{
    uint32_t init_count;
    uint32_t write_count;
    uint32_t read_reg_count;
    uint8_t last_address;
    uint8_t last_reg_addr;
    uint16_t last_length;
} device_iic_test_context_t;

static device_iic_status_t DeviceIicTest_Init(void *context)
{
    device_iic_test_context_t *test_context = (device_iic_test_context_t *)context;

    test_context->init_count++;
    return DEVICE_IIC_STATUS_OK;
}

static device_iic_status_t DeviceIicTest_Write(void *context, uint8_t address,
    const uint8_t *data, uint16_t length)
{
    device_iic_test_context_t *test_context = (device_iic_test_context_t *)context;

    assert(data != (const uint8_t *)0);
    test_context->write_count++;
    test_context->last_address = address;
    test_context->last_length = length;
    return DEVICE_IIC_STATUS_OK;
}

static device_iic_status_t DeviceIicTest_ReadReg8(void *context, uint8_t address,
    uint8_t reg_addr, uint8_t *reg_data)
{
    device_iic_test_context_t *test_context = (device_iic_test_context_t *)context;

    assert(reg_data != (uint8_t *)0);
    test_context->read_reg_count++;
    test_context->last_address = address;
    test_context->last_reg_addr = reg_addr;
    *reg_data = 0xA5U;
    return DEVICE_IIC_STATUS_OK;
}

int main(void)
{
    const device_iic_ops_t ops = {
        .init = DeviceIicTest_Init,
        .write = DeviceIicTest_Write,
        .read_reg8 = DeviceIicTest_ReadReg8,
    };
    const device_iic_ops_t write_only_ops = {
        .write = DeviceIicTest_Write,
    };
    device_iic_test_context_t test_context = {0};
    device_iic_t iic = {
        .ops = &ops,
        .context = &test_context,
        .address = 0x69U,
    };
    device_iic_t write_only_iic = {
        .ops = &write_only_ops,
        .context = &test_context,
        .address = 0x3CU,
    };
    uint8_t write_data[2] = {0x12U, 0x34U};
    uint8_t reg_data = 0U;

    assert(DeviceIic_IsValid(&iic) == true);
    assert(DeviceIic_Init(&iic) == DEVICE_IIC_STATUS_OK);
    assert(test_context.init_count == 1U);

    assert(DeviceIic_Write(&iic, write_data, (uint16_t)sizeof(write_data)) ==
        DEVICE_IIC_STATUS_OK);
    assert(test_context.write_count == 1U);
    assert(test_context.last_address == 0x69U);
    assert(test_context.last_length == 2U);

    assert(DeviceIic_ReadReg8(&iic, 0x75U, &reg_data) == DEVICE_IIC_STATUS_OK);
    assert(test_context.read_reg_count == 1U);
    assert(test_context.last_reg_addr == 0x75U);
    assert(reg_data == 0xA5U);

    assert(DeviceIic_Probe(&write_only_iic) == DEVICE_IIC_STATUS_ERROR_UNSUPPORTED);
    assert(DeviceIic_Read(&write_only_iic, &reg_data, 1U) ==
        DEVICE_IIC_STATUS_ERROR_UNSUPPORTED);
    assert(DeviceIic_Write(&iic, (const uint8_t *)0, 1U) ==
        DEVICE_IIC_STATUS_ERROR_PARAM);

    iic.address = 0x80U;
    assert(DeviceIic_IsValid(&iic) == false);
    assert(DeviceIic_Write(&iic, write_data, 1U) == DEVICE_IIC_STATUS_ERROR_PARAM);

    return 0;
}
