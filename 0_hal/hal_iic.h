/**
 * @file    hal_iic.h
 * @brief   Board-independent IIC transfer API.
 * @details The caller injects a DriverLib I2C instance; no SysConfig-generated
 *          instance or IRQ macros are required by this layer.
 * @author  cube
 * @date    2026-08-03
 * @note    Updated: 2026-08-03
 */

#ifndef __HAL_IIC_H
#define __HAL_IIC_H

#include <stdbool.h>
#include <stdint.h>

#include <ti/driverlib/dl_i2c.h>

/* HAL IIC 传输模式定义值：轮询模式。 */
#define HAL_IIC_TRANSFER_MODE_POLL      (0U)

/* HAL IIC 传输模式定义值：中断模式。 */
#define HAL_IIC_TRANSFER_MODE_IRQ       (1U)

/*
 * HAL IIC 当前传输模式选择宏。
 * 如需切换为轮询模式，请将该宏改为 HAL_IIC_TRANSFER_MODE_POLL。
 */
#ifndef HAL_IIC_TRANSFER_MODE
#define HAL_IIC_TRANSFER_MODE           HAL_IIC_TRANSFER_MODE_IRQ
#endif

/*
 * 寄存器读链路强制轮询开关。
 * 置 1 后，即便当前全局传输模式为 IRQ，hal_iic_read_reg8s() 也会改走轮询路径，
 * 便于对比判断 IMU 当前 33ms 耗时是否由 IRQ 读状态机引入。
 */
#ifndef HAL_IIC_FORCE_POLL_REG_READ
#define HAL_IIC_FORCE_POLL_REG_READ     (0U)
#endif

/*
 * When IRQ mode is selected for the current IMU project, only the following
 * controller interrupts are required in SysConfig:
 * RX Done, TX Done, RX FIFO trigger, Addr/Data NACK and Arbitration lost.
 * Register reads use two IRQ transactions:
 * first write the register address, then start a separate IRQ read.
 */

/**
  * @brief  通用 IIC 接口返回状态定义。
  */
typedef enum
{
    HAL_IIC_STATUS_OK = 0U,
    HAL_IIC_STATUS_ERROR_PARAM,
    HAL_IIC_STATUS_ERROR_TIMEOUT,
    HAL_IIC_STATUS_ERROR_BUSY,
    HAL_IIC_STATUS_ERROR_NACK,
    HAL_IIC_STATUS_ERROR_ARB_LOST,
    HAL_IIC_STATUS_ERROR_TRANSFER
} hal_iic_status_t;

/** @brief HAL 用于 profile 统计的微秒时间源，由 Board 或平台层注入。 */
typedef uint64_t (*hal_iic_time_us_fn_t)(void);

typedef struct
{
    uint16_t transfer_length;
    uint8_t last_stage;
    uint8_t finish_stage;
    uint32_t last_irq_iidx;
    uint32_t last_controller_status;
    uint32_t finish_controller_status;
    uint32_t last_raw_interrupt_status;
    uint32_t finish_raw_interrupt_status;
    uint32_t rxfifo_trigger_count;
    uint32_t rxdone_count;
    uint32_t wait_notify_value;
    uint32_t wait_rtos_ticks;
    uint32_t finish_rtos_tick;
    uint32_t wake_lag_rtos_ticks;
    uint32_t result_code;
    uint8_t completed_by_timeout;
    uint32_t wait_bus_us;
    uint32_t wait_idle_us;
    uint32_t clear_us;
    uint32_t prepare_us;
    uint32_t wait_us;
    uint32_t poll_tx_us;
    uint32_t poll_rx_us;
    uint32_t poll_tail_us;
    uint32_t total_us;
} hal_iic_profile_t;

/**
  * @brief  注册并初始化一个 IIC 控制器实例。
  * @param  iic_instance 由 Board 层提供的控制器寄存器基址。
  * @param  bus_speed_hz 该控制器的总线速率，单位为 Hz。
  * @param  time_us_fn profile 统计时间源；可传空指针以关闭时间统计。
  * @retval HAL_IIC_STATUS_OK 表示注册成功；参数非法或实例槽位耗尽时返回错误。
  */
hal_iic_status_t hal_iic_init(I2C_Regs *iic_instance, uint32_t bus_speed_hz,
    hal_iic_time_us_fn_t time_us_fn);

/**
  * @brief  获取已注册控制器的总线速率。
  * @param  iic_instance 已注册的控制器寄存器基址。
  * @retval 总线速率，单位为 Hz；未注册时返回 0。
  */
uint32_t hal_iic_get_bus_speed_hz(I2C_Regs *iic_instance);

/**
  * @brief  获取已注册控制器的调试 profile。
  * @param  iic_instance 已注册的控制器寄存器基址。
  * @retval profile 指针；未注册时返回空指针。
  */
const hal_iic_profile_t *hal_iic_get_profile(I2C_Regs *iic_instance);

/**
  * @brief  由 Board 层的 IIC ISR 转发到 HAL 状态机。
  * @param  iic_instance 触发中断的控制器寄存器基址。
  */
void hal_iic_irq_handler(I2C_Regs *iic_instance);

/* HAL IIC 基础设备访问接口。 */
hal_iic_status_t hal_iic_check_device(I2C_Regs *iic_instance, uint8_t device_addr);
hal_iic_status_t hal_iic_write_bytes(I2C_Regs *iic_instance, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length);
hal_iic_status_t hal_iic_read_bytes(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t *read_data, uint16_t read_length);

/* HAL IIC 8 位寄存器访问接口。 */
hal_iic_status_t hal_iic_write_reg8(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, uint8_t reg_data);
hal_iic_status_t hal_iic_read_reg8(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *reg_data);
hal_iic_status_t hal_iic_write_reg8s(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);
hal_iic_status_t hal_iic_read_reg8s(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);

#endif
