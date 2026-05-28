/**
 * @file stepper_driver.h
 * @brief MSPM0G3507 双步进电机底层驱动接口
 * @author cube
 * @date 2026-04-11
 *
 * 分层说明:
 * - Device 层 (本文件): 步进电机运行时状态管理、PWM 参数计算、ops 回调
 * - Board 层 (stepper_board.c): 引脚、PWM 定时器、控制定时器的板级绑定
 * - HAL 层 (hal_gpio.h): GPIO 操作通过 HAL_GPIO_* 接口
 *
 * Device 层不包含任何 SysConfig 宏或具体引脚号,
 * 所有硬件资源由 Board 层通过 Stepper_Init() 注入。
 */

#ifndef MYUSED_MYDEVICES_STEPPER_DRIVER_H
#define MYUSED_MYDEVICES_STEPPER_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "hal_gpio.h"
#include "ti_msp_dl_config.h"
#include "device_base.h"
#include "device_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 当前工程支持的步进电机数量 */
#define STEPPER_MOTOR_COUNT                    (2U)
/** @brief 默认同步反向周期，单位 ms */
#define STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS (5000U)
/** @brief 默认 STEP 高电平脉宽，单位 us */
#define STEPPER_DEFAULT_PULSE_WIDTH_US         (5U)

/**
 * @brief 步进电机编号
 */
typedef enum {
    STEPPER_MOTOR_1 = 0U,
    STEPPER_MOTOR_2 = 1U,
} Stepper_MotorId_t;

/**
 * @brief 步进电机逻辑方向
 */
typedef enum {
    STEPPER_DIR_FORWARD = 0U,
    STEPPER_DIR_REVERSE = 1U,
} Stepper_Direction_t;

/**
 * @brief 使能引脚的有效电平定义
 */
typedef enum {
    STEPPER_ENABLE_ACTIVE_LOW = 0U,
    STEPPER_ENABLE_ACTIVE_HIGH = 1U,
} Stepper_EnablePolarity_t;

/**
 * @brief 单个步进电机的板级硬件配置
 *
 * 由 Board 层根据实际 PCB 接线填充，通过 Stepper_Init() 注入 Device 层。
 * Device 层在运行期间只读，不修改这些字段。
 */
typedef struct {
    GPTIMER_Regs *pwmInst;              /**< STEP 脉冲对应的 PWM 定时器实例 */
    uint32_t pwmClockHz;                /**< PWM 定时器输入时钟，单位 Hz */
    DL_TIMER_CC_INDEX pwmCcIndex;       /**< STEP 输出使用的比较通道 */
    platform_gpio_t step;               /**< STEP GPIO */
    uint32_t stepIomux;                 /**< STEP 引脚 IOMUX */
    uint32_t stepIomuxFunc;             /**< STEP 引脚外设复用功能 */
    platform_gpio_t dir;                /**< DIR GPIO */
    platform_gpio_t en;                 /**< EN GPIO */
    Stepper_EnablePolarity_t enPolarity; /**< EN 有效电平 */
    bool forwardLevelHigh;              /**< true 表示 DIR 高电平为正向 */
} stepper_hw_cfg_t;

/**
 * @brief 步进电机控制定时器配置
 *
 * 用于 1ms 控制节拍（Stepper_Service1ms），由 Board 层提供。
 */
typedef struct {
    GPTIMER_Regs *timerInst;            /**< 控制定时器实例 */
    uint32_t loadValue;                 /**< 定时器重载值 */
    IRQn_Type irqNum;                   /**< 中断号 */
} stepper_ctrl_timer_cfg_t;

/**
 * @brief 单个步进电机的运行时状态
 *
 * 仅包含 Device 层管理的逻辑状态和计算结果。
 * 硬件绑定信息在 stepper_hw_cfg_t 中，由 Board 层管理。
 */
typedef struct {
    bool enabled;                       /**< 当前逻辑使能状态 */
    bool running;                       /**< 当前是否输出 STEP 脉冲 */
    Stepper_Direction_t direction;      /**< 当前逻辑方向 */
    uint32_t frequencyHz;               /**< 当前 STEP 频率 */
    uint32_t pulseWidthUs;              /**< 当前 STEP 高电平脉宽 */
    uint32_t periodTicks;               /**< 当前 PWM 周期计数值 */
    uint32_t pulseTicks;                /**< 当前 PWM 高电平计数值 */
    uint32_t targetFrequencyHz;         /**< 预留给加减速的目标频率 */
    uint32_t accelHzPerMs;              /**< 预留给加速规划的斜率 */
    uint32_t decelHzPerMs;              /**< 预留给减速规划的斜率 */
} Stepper_MotorState_t;

/**
 * @brief 两电机同步反向控制状态
 */
typedef struct {
    bool enabled;
    uint32_t periodMs;
    uint32_t elapsedMs;
} Stepper_SyncReverse_t;

typedef struct stepper_s stepper_t;
typedef device_stepper_ops_t stepper_base_ops_t;

struct stepper_s {
    device_base_t base;
    const stepper_base_ops_t *ops;
    Stepper_MotorId_t motor_id;
};

void Stepper_InitDevice(stepper_t *me, Stepper_MotorId_t motor_id);

/**
 * @brief 初始化步进底层驱动
 * @param hw_cfgs 双电机硬件配置数组，长度必须为 STEPPER_MOTOR_COUNT
 * @param ctrl_timer_cfg 控制定时器配置（1ms 节拍）
 * @param devices 双电机设备对象数组，由 Board 层分配并传入
 *
 * Board 层在完成 GPIO IOMUX 初始化后调用本函数，
 * 注入硬件配置、设备对象并初始化所有内部状态。
 */
void Stepper_Init(const stepper_hw_cfg_t hw_cfgs[STEPPER_MOTOR_COUNT],
                  const stepper_ctrl_timer_cfg_t *ctrl_timer_cfg,
                  stepper_t devices[STEPPER_MOTOR_COUNT]);

void Stepper_Enable(Stepper_MotorId_t motorId, bool enable);
void Stepper_EnableAll(bool enable);
void Stepper_SetEnablePolarity(Stepper_MotorId_t motorId,
                               Stepper_EnablePolarity_t polarity);
void Stepper_SetForwardLevel(Stepper_MotorId_t motorId, bool forwardLevelHigh);
void Stepper_SetDirection(Stepper_MotorId_t motorId,
                          Stepper_Direction_t direction);
void Stepper_ToggleDirection(Stepper_MotorId_t motorId);
bool Stepper_SetFrequency(Stepper_MotorId_t motorId, uint32_t frequencyHz);
bool Stepper_SetPulseWidthUs(Stepper_MotorId_t motorId, uint32_t pulseWidthUs);
void Stepper_Start(Stepper_MotorId_t motorId);
void Stepper_Stop(Stepper_MotorId_t motorId);
void Stepper_StartAll(void);
void Stepper_StopAll(void);
void Stepper_ConfigSyncReverse(bool enable, uint32_t periodMs);
void Stepper_Service1ms(void);
const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t motorId);
const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void);

#ifdef __cplusplus
}
#endif

#endif /* MYUSED_MYDEVICES_STEPPER_DRIVER_H */
