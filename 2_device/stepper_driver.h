/**
 * @file stepper_driver.h
 * @brief 实例化步进电机及控制器接口
 * @author cube
 * @date 2026-04-11
 *
 * 分层说明:
 * - Device 层 (本文件): 步进电机运行时状态管理、PWM 参数计算、ops 回调
 * - Board 层 (stepper_board.c): 引脚、PWM 定时器、控制定时器的板级绑定
 * - HAL 层 (hal_gpio.h): GPIO 操作通过 HAL_GPIO_* 接口
 *
 * Device 层不包含任何 SysConfig 宏或具体引脚号,
 * 所有硬件资源由 Board 层通过 Stepper_ControllerInit() 注入。
 */

#ifndef MYUSED_MYDEVICES_STEPPER_DRIVER_H
#define MYUSED_MYDEVICES_STEPPER_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "hal_gpio.h"
#include "hal_timer.h"
#include "device_base.h"
#include "device_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * 由 Board 层注入，复制到各电机对象；极性可在运行期间配置。
 */
typedef struct {
    platform_timer_t *pwmInst;          /**< STEP 脉冲对应的 PWM 定时器实例 */
    uint32_t pwmClockHz;                /**< Counter clock after prescaler, Hz; Board configures edge-aligned PWM1. */
    platform_timer_channel_t pwmCcIndex; /**< STEP 输出使用的比较通道 */
    platform_gpio_t step;               /**< STEP GPIO */
    uint32_t stepAlternate;             /**< STEP 引脚 STM32 alternate function */
    platform_gpio_t dir;                /**< DIR GPIO */
    platform_gpio_t en;                 /**< EN GPIO */
    Stepper_EnablePolarity_t enPolarity; /**< EN 有效电平 */
    bool forwardLevelHigh;              /**< true 表示 DIR 高电平为正向 */
} stepper_hw_cfg_t;

/**
 * @brief 步进电机控制定时器配置
 *
 * 用于 1ms 控制节拍，由 Board 层配置定时器和 IRQ 并调用 controller service。
 */
typedef struct {
    platform_timer_t *timerInst;        /**< 控制定时器实例 */
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
 * @brief 一个控制器下全部电机的同步反向控制状态
 */
typedef struct {
    bool enabled;
    uint32_t periodMs;
    uint32_t elapsedMs;
} Stepper_SyncReverse_t;

typedef struct stepper_s stepper_t;
typedef struct stepper_controller_s stepper_controller_t;
typedef device_stepper_ops_t stepper_base_ops_t;

struct stepper_s {
    device_base_t base;
    const stepper_base_ops_t *ops;
    stepper_controller_t *controller;
    uint32_t index;
    stepper_hw_cfg_t cfg;
    Stepper_MotorState_t state;
};

/** @brief 调用者持有的设备集合；初始化前必须清零，禁止按值复制活动对象。 */
struct stepper_controller_s {
    device_base_t base;
    stepper_t *devices;
    uint32_t count;
    stepper_ctrl_timer_cfg_t ctrl_timer_cfg;
    Stepper_SyncReverse_t sync_reverse;
};

/**
 * @brief 初始化任意数量的电机；所有配置通过校验后才绑定对象和驱动 GPIO。
 * @param me 已清零且未初始化的控制器。
 * @param devices 已清零且未绑定的数组，生命周期覆盖控制器使用期。
 * @param count 数组元素数；必须大于零。
 * @param hw_cfgs 配置数组，长度为 count；配置复制到设备内。
 * @param ctrl_timer_cfg 可为 NULL，使用外部 1ms 调度；定时器/IRQ 由 Board 启动。
 * @return 配置无效或对象已初始化时返回 false，不改动现有对象。
 */
bool Stepper_ControllerInit(stepper_controller_t *me, stepper_t *devices,
                            uint32_t count, const stepper_hw_cfg_t *hw_cfgs,
                            const stepper_ctrl_timer_cfg_t *ctrl_timer_cfg);
/** @brief 停止并失能所有电机，释放数组绑定；Board 须先停止服务和控制 IRQ。 */
void Stepper_ControllerDeinit(stepper_controller_t *me);
/** @brief 按索引获取对象；未初始化或越界时返回 NULL。 */
stepper_t *Stepper_ControllerGetDevice(stepper_controller_t *me, uint32_t index);
/** @brief 控制器批量使能，NULL/未初始化时无操作。 */
void Stepper_ControllerEnableAll(stepper_controller_t *me, bool enable);
/** @brief 启动所有已使能且时序有效的电机。 */
void Stepper_ControllerStartAll(stepper_controller_t *me);
/** @brief 停止控制器所有 STEP 输出。 */
void Stepper_ControllerStopAll(stepper_controller_t *me);
/** @brief 配置本控制器的同步反向；周期零使用默认值。 */
void Stepper_ControllerConfigSyncReverse(stepper_controller_t *me, bool enable,
                                         uint32_t period_ms);
/** @brief 每控制器每毫秒调用一次；不要对每个电机重复调用。 */
void Stepper_ControllerService1ms(stepper_controller_t *me);
/** @brief 获取控制器只读同步状态；无效对象返回 NULL。 */
const Stepper_SyncReverse_t *Stepper_ControllerGetSyncReverseState(const stepper_controller_t *me);
/** @brief 设置电机使能；失能会先停止 STEP 输出。 */
void Stepper_DeviceEnable(stepper_t *me, bool enable);
/** @brief 配置 EN 有效电平；非法枚举不生效。 */
void Stepper_DeviceSetEnablePolarity(stepper_t *me, Stepper_EnablePolarity_t polarity);
/** @brief 配置正向对应的 DIR 电平。 */
void Stepper_DeviceSetForwardLevel(stepper_t *me, bool high);
/** @brief 设置逻辑方向；运行时暂停输出后切换。 */
void Stepper_DeviceSetDirection(stepper_t *me, Stepper_Direction_t direction);
/** @brief 翻转电机逻辑方向。 */
void Stepper_DeviceToggleDirection(stepper_t *me);
/** @brief 设置 STEP 频率，非法值返回 false 且保持旧时序。 */
bool Stepper_DeviceSetFrequency(stepper_t *me, uint32_t frequency_hz);
/** @brief 设置 STEP 脉宽，非法值返回 false 且保持旧时序。 */
bool Stepper_DeviceSetPulseWidthUs(stepper_t *me, uint32_t pulse_width_us);
/** @brief 启动已使能且时序有效的电机。 */
void Stepper_DeviceStart(stepper_t *me);
/** @brief 立即停止 STEP 并拉低引脚。 */
void Stepper_DeviceStop(stepper_t *me);
/** @brief 获取电机只读状态；无效对象返回 NULL。 */
const Stepper_MotorState_t *Stepper_DeviceGetState(const stepper_t *me);

#ifdef __cplusplus
}
#endif

#endif /* MYUSED_MYDEVICES_STEPPER_DRIVER_H */
