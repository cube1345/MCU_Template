# 项目外设分层架构说明

## 1. 文档目的

本文档用于系统说明当前项目外设驱动的分层架构设计，包括：

- 五层架构分别承担什么职责；
- 为什么要将外设拆分为多层；
- 各类外设在当前架构中的功能划分；
- `Core`、`Platform/BSP`、`Device`、`Board`、`Facade/Compat` 等层的意义；
- 后续新增外设时应遵守的设计原则。

当前工程已经将 LED、Key、Encoder、Buzzer、OLED、ICM42688、MPU6050、Xunji、Motor、Stepper 等模块逐步整理为类似的分层结构。

---

## 2. 总体架构概览

当前外设驱动采用五层思想组织：

```text
Application / App / Libs
        |
        v
Facade / Compat 层
        |
        v
Board 层
        |
        v
Device 层
        |
        v
Core + Platform/BSP 层
        |
        v
MCU 外设寄存器 / SysConfig / DriverLib
```

实际目录大致对应如下：

| 层级 | 目录 | 职责 |
| --- | --- | --- |
| Core 层 | `2_core` | 通用设备基类、通用操作接口定义 |
| Platform/BSP 层 | `1_platform`、`0_bsp` | GPIO/I2C/SPI 等平台适配与底层硬件访问 |
| Device 层 | `3_device` | 外设对象、配置结构体、默认行为、ops 分发 |
| Board 层 | `4_board` | 绑定具体板级资源、SysConfig 宏、具体实例 |
| Facade 层 | `5_facade` | 面向业务层的兼容 API、简化接口 |
| Compat 层 | `5_compat`、部分 `4_board` | 保持旧 include 路径和旧 API 兼容 |

> 注：严格来说 `Compat` 可以看作 Facade 的一部分，也可以看作迁移期的第六类辅助层。本文为了说明迁移过程，会单独介绍。

---

## 3. 为什么需要五层架构

### 3.1 避免驱动文件职责混乱

早期单文件驱动常常同时包含：

- 硬件寄存器操作；
- GPIO/I2C/SPI 绑定；
- 设备状态变量；
- 业务接口；
- 中断处理；
- 全局兼容变量。

这样会导致：

1. 一个文件越来越大；
2. 外设逻辑和板级资源强耦合；
3. 换引脚、换定时器、换总线时容易改坏设备算法；
4. 业务层直接依赖底层实现；
5. 很难复用同一类设备的通用逻辑。

五层架构的核心目标就是将这些职责拆开。

### 3.2 提高复用能力

例如 `LED`、`Buzzer` 本质上都是开关型设备，它们可以复用 `device_switch_ops_t` 的思想；`Key` 是输入设备；`Encoder` 是计数设备；`Motor` 是执行器；`ICM42688` 是传感器。

通过抽象 `ops`，可以让不同设备拥有统一的对象调用方式，同时保留各自的具体行为。

### 3.3 降低板级迁移成本

Device 层不应该关心：

- 当前 LED 接在哪个 GPIO；
- 当前 PWM 使用哪个 Timer；
- 当前 I2C 使用哪个 BSP 端口；
- 当前中断号是什么。

这些都属于 Board 层职责。

当 PCB 或 SysConfig 变化时，优先修改 Board 层，而不是修改 Device 层算法。

### 3.4 兼容旧业务代码

当前项目中已经存在不少旧接口，例如：

- `LED_B_On()`；
- `Buzzer_Start()`；
- `Key_GetValue()`；
- `Encoder_Init()`；
- `OLED_Init()`；
- `Xunji_Get()`；
- `Set_PWM()`；
- `Stepper_Start()`；
- `bsp_Icm42688Init()`。

如果直接删除旧接口，会造成业务层大面积修改。因此引入 Facade/Compat 层，让旧代码继续可用，同时内部逐步转向新架构。

---

## 4. Core 层

### 4.1 位置

核心文件：

- `2_core/device_base.h`
- `2_core/device_ops.h`

### 4.2 `device_base_t`

`device_base_t` 是所有设备对象的最小公共基类，目前主要保存初始化状态：

```c
typedef struct {
    uint8_t is_init;
} device_base_t;
```

每个 Device 对象都应包含：

```c
device_base_t base;
```

这样每个设备都可以统一判断是否已经初始化。

### 4.3 `device_ops.h`

`device_ops.h` 定义不同设备类型的通用操作集合。

当前包括：

| ops 类型 | 适用设备 | 典型操作 |
| --- | --- | --- |
| `device_switch_ops_t` | LED、Buzzer | `on`、`off`、`toggle` |
| `device_input_ops_t` | Key | `scan`、`read_level` |
| `device_counter_ops_t` | Encoder | `get_count`、`reset_count`、`update_from_ab` |
| `device_display_ops_t` | OLED | `init`、`update`、`clear` |
| `device_sensor_ops_t` | ICM42688 | `init`、`read_raw`、`read_scaled` |
| `device_array_input_ops_t` | Xunji | `read_all` |
| `device_motor_ops_t` | Motor | `start`、`stop`、`set_pwm`、`velocity_calc` |
| `device_stepper_ops_t` | Stepper | `enable`、`set_direction`、`set_frequency`、`start`、`stop`、`service` |

### 4.4 Core 层意义

Core 层不依赖具体硬件，不依赖 SysConfig，不依赖某个具体外设。

它提供的是：

- 统一对象模型；
- 统一初始化状态；
- 统一操作接口风格；
- 后续扩展新设备时的基础约束。

---

## 5. Platform/BSP 层

### 5.1 位置

主要目录：

- `1_platform`
- `0_bsp`

典型文件：

- `platform_gpio.h/.c`
- `bsp_gpio.h/.c`
- `bsp_iic.h/.c`
- `bsp_spi.h/.c`

### 5.2 BSP 层职责

BSP 层负责直接对接 MCU DriverLib、寄存器或 SysConfig 生成宏。

例如：

- GPIO 读写；
- I2C 读写寄存器；
- SPI 传输；
- Timer/PWM 控制。

### 5.3 Platform 层职责

Platform 层比 BSP 更接近通用设备抽象。

例如 `platform_gpio_t` 将 GPIO 端口和引脚包装为结构体，让 Device 层可以通过统一接口读写 GPIO，而不直接依赖 `DL_GPIO_setPins()` 等底层函数。

### 5.4 Platform/BSP 层意义

这一层隔离 MCU 厂商库和设备逻辑。

如果将来从 MSPM0G3507 迁移到其他 MCU，理论上优先改 BSP/Platform，而 Device 层逻辑可以尽量保持稳定。

---

## 6. Device 层

### 6.1 位置

目录：

- `3_device`

### 6.2 Device 层职责

Device 层是当前架构中最核心的外设类实现层。

每个外设通常包含：

1. 对象结构体；
2. 配置结构体；
3. ops 类型别名；
4. 默认 ops 实现；
5. 初始化函数；
6. 对象方法；
7. 必要的状态变量。

典型形式：

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    // 外设配置
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
    // 运行时状态
};
```

### 6.3 Device 层不应该做什么

Device 层原则上不应该强依赖：

- 某一个具体板子的引脚编号；
- 某一个具体定时器实例；
- 某一个业务场景；
- 某一个 App 的全局变量。

如果必须使用硬件资源，应尽量通过 `cfg` 或回调注入。

---

## 7. Board 层

### 7.1 位置

目录：

- `4_board`

### 7.2 Board 层职责

Board 层负责把 Device 层的通用设备类绑定到当前硬件板。

典型职责包括：

- 创建具体对象实例；
- 填写 GPIO 配置；
- 填写 Timer/PWM 配置；
- 绑定 I2C/SPI 端口；
- 绑定 SysConfig 生成的宏；
- 配置 NVIC；
- 放置强板级 ISR。

例如 Encoder 的板级层负责：

- 实例化 Encoder A/B；
- 绑定 A/B 相 GPIO；
- 在 `GROUP1_IRQHandler` 中读取中断状态和引脚电平；
- 调用 `Encoder_UpdateFromAB()` 更新计数。

### 7.3 Board 层意义

Board 层是“硬件差异集中区”。

当硬件引脚变化、PWM 通道变化、I2C 地址变化时，应优先修改 Board 层。

---

## 8. Facade 层

### 8.1 位置

目录：

- `5_facade`

### 8.2 Facade 层职责

Facade 层面向业务代码，提供简单稳定的接口。

它隐藏对象指针、Board 实例、ops 调用细节，让业务层只调用熟悉的函数。

例如：

```c
LED_B_On();
Key_GetValue();
Encoder_GetCountA();
Buzzer_Start();
```

Facade 内部再转调 Board 层对象和 Device 层方法。

### 8.3 Facade 层意义

Facade 层的价值包括：

1. 业务层接口稳定；
2. 降低业务层认知成本；
3. 隐藏对象化实现；
4. 便于兼容旧 API；
5. 后续重构 Device/Board 时减少业务层改动。

---

## 9. Compat 层

### 9.1 位置

主要是：

- `5_compat`
- 部分 `4_board`

### 9.2 Compat 层职责

Compat 层是迁移期保留的兼容壳。

旧代码可能仍然包含：

```c
#include "motor_driver.h"
#include "OLED_driver.h"
#include "xunji_driver.h"
```

如果直接移动文件，会导致旧 include 失效。

因此旧 `5_compat/*.h` 现在主要转发到 `5_facade/*.h`，旧 `.c` 则保留为空翻译单元，避免重复定义。

### 9.3 Compat 层意义

Compat 层让架构重构可以渐进完成，而不是一次性修改所有业务代码。

长期来看，推荐新代码逐步直接 include Facade 或 Device/Board 头文件，而不是继续依赖旧 Drivers 路径。

---

## 10. 当前各外设功能划分

## 10.1 LED

### Device 层

文件：

- `3_device/led_driver.h`
- `3_device/led_driver.c`

职责：

- 定义 `led_t`；
- 定义 `led_cfg_t`；
- 继承 `device_switch_ops_t`；
- 实现 `LED_Init()`、`LED_On()`、`LED_Off()`、`LED_Toggle()`；
- 根据 `active_low` 处理实际输出电平。

### Board 层

文件：

- `4_board/led_board.h`
- `4_board/led_board.c`

职责：

- 实例化蓝灯、红灯；
- 绑定 `LED_BLUE_PORT`、`LED_BLUE_PIN` 等 SysConfig 宏；
- 提供 `LED_Board_GetBlue()`、`LED_Board_GetRed()`。

### Facade 层

文件：

- `5_facade/led_facade.h`
- `5_facade/led_facade.c`

职责：

- 保留 `LED_B_On()`、`LED_R_On()` 等业务接口。

---

## 10.2 Buzzer

### Device 层

文件：

- `3_device/buzzer_driver.h`
- `3_device/buzzer_driver.c`

职责：

- 定义 `buzzer_t`；
- 定义 `buzzer_cfg_t`；
- 继承 `device_switch_ops_t`；
- 实现 `Buzzer_InitDevice()`、`Buzzer_On()`、`Buzzer_Off()`；
- 根据 `active_high` 处理蜂鸣器实际电平。

### Board 层

文件：

- `4_board/buzzer_board.h`
- `4_board/buzzer_board.c`

职责：

- 实例化蜂鸣器对象；
- 绑定蜂鸣器 GPIO。

### Facade 层

文件：

- `5_facade/buzzer_facade.h`
- `5_facade/buzzer_facade.c`

职责：

- 保留 `Buzzer_Start()`、`Buzzer_Stop()`、`Buzzer_Init()` 等接口。

---

## 10.3 Key

### Device 层

文件：

- `3_device/key_driver.h`
- `3_device/key_driver.c`

职责：

- 定义 `key_t`；
- 定义 `key_cfg_t`；
- 继承 `device_input_ops_t`；
- 实现按键状态机；
- 支持消抖、短按、长按、连发；
- 通过 `get_tick` 回调获取系统时间。

### Board 层

文件：

- `4_board/key_board.h`
- `4_board/key_board.c`

职责：

- 实例化多个按键；
- 绑定 GPIO；
- 绑定系统 tick。

### Facade 层

文件：

- `5_facade/key_facade.h`
- `5_facade/key_facade.c`

职责：

- 保留 `Key_Init()`、`Key_GetValue()`。

---

## 10.4 Encoder

### Device 层

文件：

- `3_device/encoder_driver.h`
- `3_device/encoder_driver.c`

职责：

- 定义 `encoder_t`；
- 定义 `encoder_cfg_t`；
- 继承 `device_counter_ops_t`；
- 实现 `Encoder_GetCount()`、`Encoder_ResetCount()`、`Encoder_UpdateFromAB()`；
- 通过 A/B 相变化更新计数。

### Board 层

文件：

- `4_board/encoder_board.h`
- `4_board/encoder_board.c`

职责：

- 实例化 Encoder A/B；
- 绑定 A/B 相 GPIO；
- 配置并响应 GPIO 中断；
- 在 ISR 中调用 Device 层更新计数。

### Facade 层

文件：

- `5_facade/encoder_facade.h`
- `5_facade/encoder_facade.c`

职责：

- 保留 `Encoder_Init()`、`Encoder_GetCountA()`、`Encoder_GetCountB()` 等接口；
- 兼容旧全局计数变量。

---

## 10.5 OLED

### Device 层

文件：

- `3_device/OLED_driver.h`
- `3_device/OLED_driver.c`

职责：

- 保留 OLED 显存和绘图函数；
- 定义 `oled_t`、`oled_cfg_t`；
- 继承 `device_display_ops_t`；
- 封装 `OLED_InitDevice()`、`OLED_GetBaseOps()`；
- 通过 `OLED_SCL`、`OLED_SDA` 回调绑定软件 I2C 线控制。

### Board 层

文件：

- `4_board/oled_board.h`
- `4_board/oled_board.c`

职责：

- 提供 OLED 板级初始化入口；
- 后续可进一步迁移 `OLED_int.c` 中的 SCL/SDA GPIO 写函数。

### Facade/Compat 层

文件：

- `5_facade/oled_facade.h`
- `5_compat/OLED_driver.h`

职责：

- 保持 `OLED_Init()`、`OLED_Update()`、`OLED_Clear()`、`OLED_ShowString()` 等原 API 可用。

---

## 10.6 ICM42688

### Device 层

文件：

- `3_device/icm42688_driver.h`
- `3_device/icm42688_driver.c`

职责：

- 保留寄存器读写、配置、数据换算算法；
- 定义 `icm42688_t`、`icm42688_cfg_t`；
- 继承 `device_sensor_ops_t`；
- 封装 `ICM42688_InitDevice()`；
- 封装 `ICM42688_ReadRawDevice()`；
- 封装 `ICM42688_ReadScaledDevice()`。

### Board 层

文件：

- `4_board/icm42688_board.h`
- `4_board/icm42688_board.c`

职责：

- 提供板级初始化入口；
- 后续可进一步将 I2C 端口、器件地址等从 `icm42688_int` 迁入 Board 层。

### Facade/Compat 层

文件：

- `5_facade/icm42688_facade.h`
- `5_compat/icm42688_driver.h`

职责：

- 保留 `icm42688_init()`、`bsp_Icm42688Init()`、`bsp_IcmGetAccelerometer()` 等旧接口。

---

## 10.7 MPU6050

### Device 层

文件：

- `3_device/mpu6050_driver.h`
- `3_device/mpu6050_driver.c`

职责：

- 保留寄存器读写、配置、数据换算算法；
- 定义 `mpu6050_t`、`mpu6050_cfg_t`；
- 继承 `device_sensor_ops_t`；
- 封装 `MPU6050_InitDevice()`；
- 封装 `MPU6050_ReadRawDevice()`；
- 封装 `MPU6050_ReadScaledDevice()`；
- 提供 `mpu6050_read_raw_data()`、`mpu6050_convert_raw_data()`。

### Board/Int 层

文件：

- `4_board/mpu6050_int.h`
- `4_board/mpu6050_int.c`
- `4_board/mpu6050_board.h`
- `4_board/mpu6050_board.c`

职责：

- 绑定 I2C 总线端口（`BSP_IIC_PORT_0`）和器件地址（`0x68`）；
- 通过 `mpu6050_int_*` 函数封装 `bsp_iic_*` 调用；
- 提供 `MPU6050_Board_Init()` 板级初始化入口。

### Facade 层

文件：

- `5_facade/mpu6050_facade.h`

职责：

- 提供 header-only 转发到 Device 层，供业务层统一 include。

---

## 10.8 Xunji 循迹模块

### Device 层

文件：

- `3_device/xunji_driver.h`
- `3_device/xunji_driver.c`

职责：

- 定义 `xunji_t`；
- 定义 `xunji_cfg_t`；
- 继承 `device_array_input_ops_t`；
- 封装多路输入读取；
- 保留 `Xunji_Driver_Bind()`、`Xunji_Get()`、`Xunji_Init()`。

### Board/Int 层

文件：

- `4_board/xunji_board.h`
- `4_board/xunji_board.c`
- `4_board/xunji_int.h`
- `4_board/xunji_int.c`

职责：

- 绑定五路循迹传感器 GPIO；
- 将每一路 GPIO 读函数注册给 Device 层。

---

## 10.9 Motor 直流电机

### Device 层

文件：

- `3_device/motor_driver.h`
- `3_device/motor_driver.c`

职责：

- 定义 `motor_t`；
- 定义 `motor_cfg_t`；
- 继承 `device_motor_ops_t`；
- 封装 PWM 输出；
- 封装启动死区处理；
- 封装 PWM 限幅；
- 封装速度 PI 计算；
- 保留 `Motor_Init()`、`Set_PWM()`、`Velocity_A()`、`Velocity_B()` 等旧接口。

### Board 层

文件：

- `4_board/motor_board.h`
- `4_board/motor_board.c`

职责：

- 提供电机板级初始化入口；
- 后续可进一步将 PWM Timer/Channel 绑定从 Device 默认配置中迁入 Board 层。

---

## 10.10 Stepper 步进电机

### Device 层

文件：

- `3_device/stepper_driver.h`
- `3_device/stepper_driver.c`

职责：

- 保留双步进电机底层状态表；
- 定义 `stepper_t`；
- 继承 `device_stepper_ops_t`；
- 封装使能、方向、频率、启动、停止、服务节拍；
- 保留 `Stepper_Init()`、`Stepper_Start()`、`Stepper_Stop()` 等旧接口；
- 保留 `STEP_CTRL_TIMER_INST_IRQHandler()`。

### Board 层

文件：

- `4_board/stepper_board.h`
- `4_board/stepper_board.c`

职责：

- 提供步进电机板级初始化入口；
- 后续建议将定时器实例、GPIO、ISR 进一步迁移到 Board 层。

---

## 11. 当前架构的调用链示例

### 11.1 LED 调用链

```text
业务层 LED_B_On()
    -> Facade: led_facade.c
        -> Board: LED_Board_GetBlue()
            -> Device: LED_On(led_t *)
                -> ops->on()
                    -> Platform_GPIO_Write()
                        -> BSP / DriverLib
```

### 11.2 Encoder 调用链

```text
GPIO 中断 GROUP1_IRQHandler
    -> Board: 读取中断状态和 A/B 相电平
        -> Device: Encoder_UpdateFromAB()
            -> 更新 encoder_t.count

业务层 Encoder_GetCountA()
    -> Facade
        -> Board: Encoder_Board_GetA()
            -> Device: Encoder_GetCount()
```

### 11.3 Motor 调用链

```text
业务层 Set_PWM(left, right)
    -> Device 兼容 API
        -> Motor_SetPwmDevice(&g_default_motor, left, right)
            -> deadzone / limit
                -> DL_Timer_setCaptureCompareValue()
```

---

## 12. 新增外设推荐模板

新增一个外设时，推荐按以下步骤：

1. 判断外设类型：
   - 开关类；
   - 输入类；
   - 计数类；
   - 传感器类；
   - 显示类；
   - 执行器类。

2. 在 `device_ops.h` 中复用或新增合适的 ops。

3. 在 `3_device` 中创建：

```text
xxx_driver.h
xxx_driver.c
```

4. 在 Device 头文件中定义：

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    // 配置项
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
};
```

5. 在 `4_board` 中创建：

```text
xxx_board.h
xxx_board.c
```

用于绑定当前板子的 GPIO、Timer、I2C、SPI、中断等。

6. 在 `5_facade` 中创建：

```text
xxx_facade.h
xxx_facade.c
```

用于对业务层提供稳定 API。

7. 如果旧代码已经使用 `5_compat/xxx_driver.h`，则在 `5_compat` 保留兼容壳。

---

## 13. 当前架构的优点

### 13.1 可维护性更高

每一层职责明确，修改板级硬件不容易影响设备算法。

### 13.2 复用性更强

Device 层对象可以复用到多个实例，例如多个 LED、多个 Key、多个 Encoder。

### 13.3 业务接口稳定

Facade 和 Compat 层保留旧 API，减少业务层大面积修改。

### 13.4 便于测试

Device 层可以通过 mock cfg、mock 回调进行测试，而不一定依赖真实硬件。

### 13.5 便于扩展

新增外设可以照着已有模板实现，统一 `base + ops + cfg + board + facade` 模式。

---

## 14. 当前各层设计理念与后续开发要求

本节是后续开发、维护和新增外设时必须优先遵守的规则。项目虽然采用五层架构，但分层不是为了让文件数量变多，而是为了让每一类代码只承担清晰职责。

### 14.1 总体原则

1. **Core 层只定义规则，不定义具体设备行为。**
2. **Device 层实现外设本体，不绑定具体板级资源。**
3. **Board 层绑定当前硬件板，不承载复杂算法。**
4. **Facade 层服务业务代码，不操作寄存器、不写硬件细节。**
5. **Compat 层只为旧代码过渡存在，不应继续扩展新功能。**
6. **App 层作为模板示例保留，默认不主动调用。**

推荐调用方向：

```text
App / Libs
    -> Facade
        -> Board
            -> Device
                -> Platform / BSP
                    -> DriverLib / Registers
```

原则上不要反向依赖。例如 Device 层不应该 include App 层，Core 层不应该 include Board 层。

---

### 14.2 Core 层开发要求

目录：`2_core`

Core 层目前主要包含：

- `device_base.h`
- `device_ops.h`

Core 层设计理念：

```text
抽象共性，限制边界，提供统一对象模型。
```

#### 14.2.1 `device_base_t`

所有对象化设备都应包含：

```c
device_base_t base;
```

用于表示通用设备状态，例如是否初始化。

后续如果要扩展通用状态，应该谨慎，只能放所有设备都能接受的字段，例如：

```c
uint8_t is_init;
uint8_t error_code;
```

不应把某个外设专属状态放入 `device_base_t`。

#### 14.2.2 `device_ops.h` 的使用原则

`device_ops.h` 中的 ops 是“类别级最小公共能力”，不是具体外设完整 API。

例如显示类设备：

```c
device_display_ops_t
```

只注册：

```c
init
update
clear
```

这并不代表 OLED 只能做这些事情。OLED 的字符串显示、图形绘制、图片显示等专用能力仍然放在 `OLED_driver.h/.c` 中。

判断一个函数是否应该进入通用 ops，应满足：

1. 同类设备大多数都有这个能力；
2. 语义稳定，不强依赖某个具体芯片；
3. 参数形式足够通用；
4. 上层确实需要通过抽象接口统一调用。

不适合进入通用 ops 的例子：

```c
OLED_DrawArc();
OLED_ShowChinese();
icm42688_set_gyro_config();
Stepper_ConfigSyncReverse();
```

这些应该保留为具体设备 API。

#### 14.2.3 不要把 Core 层做成万能层

禁止在 Core 层中加入：

- SysConfig 宏；
- GPIO/I2C/SPI 具体句柄；
- 某个具体外设的寄存器；
- 业务层枚举；
- App 层状态变量。

---

### 14.3 Platform / BSP 层开发要求

目录：

- `1_platform`
- `0_bsp`

设计理念：

```text
Platform 提供较薄的跨设备硬件访问包装；BSP 负责直接接触底层外设库和寄存器。
```

#### 14.3.1 BSP 层

BSP 层可以直接使用：

- TI DriverLib；
- SysConfig 生成宏；
- 寄存器结构体；
- I2C/SPI/UART/Timer 等底层外设。

但 BSP 层不应该知道业务含义。例如 BSP 可以知道“写某个 GPIO”，但不应该知道“这是蓝灯”或“这是蜂鸣器”。

#### 14.3.2 Platform 层

Platform 层应提供轻量统一接口，例如：

```c
Platform_GPIO_Write();
Platform_GPIO_Read();
Platform_GPIO_Toggle();
```

Device 层优先调用 Platform 层，而不是直接调用 DriverLib。

---

### 14.4 Device 层开发要求

目录：`3_device`

设计理念：

```text
Device 层实现设备类本体，是外设行为、状态和配置的核心承载层。
```

每个对象化设备推荐包含：

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    // 设备配置
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
    // 实例运行状态
};
```

#### 14.4.1 实例参数必须跟着 `me` 走

如果一个参数属于某个具体设备实例，就必须放入：

- `xxx_cfg_t`
- 或 `xxx_t` 的运行状态字段

例如 Motor 的 PID 参数：

```c
kp
ki
kd
```

属于电机实例配置，因此应放在 `motor_cfg_t` 中，而不是暴露为全局变量。

#### 14.4.2 模块私有变量必须使用 `static`

只在本 `.c` 文件内部使用的变量必须写成：

```c
static xxx_t g_default_xxx;
```

不要暴露普通全局变量。

例如 OLED 显存只在 `OLED_driver.c` 内部使用，应写成：

```c
static uint8_t OLED_DisplayBuf[8][128];
```

#### 14.4.3 只读默认配置必须使用 `static const`

默认配置、只读查找表、默认参数表应写成：

```c
static const xxx_cfg_t g_default_xxx_cfg = { ... };
```

这样可以表达“不应该被运行时修改”，也利于编译器优化和放入只读段。

#### 14.4.4 内部工具函数必须使用 `static` 或 `static inline`

只在本文件使用的工具函数不允许出现在 `.h` 中。

例如：

```c
static inline int32_t Motor_LimitValue(...);
static void Stepper_ApplyPwmRegisters(...);
```

`*_GetBaseOps()` 这类内部 ops getter 只服务本文件对象初始化，也应为：

```c
static inline const xxx_base_ops_t *XXX_GetBaseOps(void);
```

#### 14.4.5 Device 层不应该做什么

Device 层不应直接承担：

- 业务流程；
- 菜单逻辑；
- 任务调度；
- App 层状态；
- 与某块板强绑定的初始化流程。

如果目前某些旧驱动仍有强板级绑定，应后续逐步迁移到 Board 层。

---

### 14.5 Board 层开发要求

目录：`4_board`

设计理念：

```text
Board 层负责把通用 Device 实例绑定到当前这块板子的具体硬件资源。
```

Board 层可以包含：

- SysConfig 宏；
- GPIO 端口和引脚；
- Timer/PWM 实例；
- I2C/SPI/UART 端口；
- 当前板子的默认配置；
- ISR 中与板级资源强相关的入口。

#### 14.5.1 Board 层负责实例化设备

例如 LED Board 层可以有：

```c
static led_t g_led_blue;
static led_t g_led_red;
```

并提供：

```c
led_t *LED_Board_GetBlue(void);
led_t *LED_Board_GetRed(void);
```

#### 14.5.2 Board 层负责硬件默认配置

例如某个 OLED 的 SCL/SDA 绑定，某个电机 PWM 通道绑定，某个按键 GPIO 绑定，都应优先放入 Board 层。

#### 14.5.3 Board 层不应该写复杂算法

Board 层只做绑定，不做业务算法。

例如：

- 编码器 AB 相计数算法属于 Device；
- 小车速度控制属于 App；
- IMU 姿态融合属于 Lib/App；
- GPIO 中断入口和引脚判断可以在 Board。

---

### 14.6 Facade 层开发要求

目录：`5_facade`

设计理念：

```text
Facade 层给业务层和旧代码提供稳定、简单、兼容的入口。
```

Facade 层适合放：

1. 面向业务的简化函数；
2. 隐藏对象指针的函数；
3. 多个 Board/Device 操作的组合；
4. 旧 API 名称的兼容实现；
5. 常用动作封装。

例如 LED：

```c
void LED_B_On(void)
{
    LED_On(LED_Board_GetBlue());
}
```

例如 Key：

```c
Key_GetValue();
```

内部扫描所有按键对象并返回第一个有效事件。

#### 14.6.1 Facade 可以是 Header-only

如果某个模块暂时没有额外门面逻辑，Facade 可以只做头文件转发。

例如 OLED 和 ICM42688 当前就是这种情况：

```c
#include "../3_device/OLED_driver.h"
#include "../3_device/icm42688_driver.h"
```

这不是错误，表示暂时没有必要额外包一层函数。

#### 14.6.2 不要机械包装所有函数

不要为了“Facade 层有代码”而把所有 Device 函数完整复制一遍。

不推荐：

```c
OLED_Facade_ShowString() -> OLED_ShowString()
OLED_Facade_DrawLine()   -> OLED_DrawLine()
OLED_Facade_DrawCircle() -> OLED_DrawCircle()
```

除非它确实隐藏了对象、简化了参数、组合了常用动作或兼容旧接口。

#### 14.6.3 Facade 不操作寄存器

Facade 层不允许直接写：

```c
DL_GPIO_setPins();
DL_Timer_setCaptureCompareValue();
```

这些应属于 Board / Device / Platform / BSP。

---

### 14.7 Compat 层开发要求

目录：`5_compat`，以及部分历史 `4_board`

设计理念：

```text
Compat 层只为历史路径和旧 API 过渡存在，不是新功能开发位置。
```

#### 14.7.1 `5_compat/*.h`

推荐作为兼容头文件保留，例如：

```c
#ifndef MYUSED_MYDRIVERS_MOTOR_DRIVER_COMPAT_H
#define MYUSED_MYDRIVERS_MOTOR_DRIVER_COMPAT_H

#include "../5_facade/motor_facade.h"

#endif
```

这样旧代码仍然可以：

```c
#include "motor_driver.h"
```

但实际转到 Facade / Device。

#### 14.7.2 `5_compat/*.c`

旧兼容 `.c` 如果只包含一个头文件，没有实际实现，建议从工程中移除并删除。

删除前必须同步修改 Keil 工程，否则 Keil 会报文件不存在。

#### 14.7.3 旧 `MyInts` → `4_board` 迁移

旧 `MyInts` 目录中的板级绑定逻辑（ICM42688 I2C 地址与端口、OLED 软件 I2C 线控、Xunji GPIO 读取等）已完成迁移，对应文件现在位于 `4_board/` 目录下（如 `4_board/icm42688_int.h/.c`、`4_board/OLED_int.h/.c`、`4_board/xunji_int.h/.c`）。

新增外设的 int 绑定文件也应直接放在 `4_board/` 中（如 `mpu6050_int.h/.c`），不再需要单独的 `MyInts` 目录。

---

### 14.8 App 层开发要求

目录：`7_apps`

设计理念：

```text
本项目是模板性质项目，App 层代码作为示例保留，默认不主动调用。
```

当前 App 示例包括：

- `car_app`：差速小车左右轮速度控制；
- `imu_app`：IMU 应用代码；
- `usart_app`：串口协议收发；
- `xunji_app`：循迹黑线检测后的数据应用；
- `yuntai_app`：双步进电机二维云台。

App 层可以组合多个 Facade/Device/Lib，但不应反过来被底层依赖。

#### 14.8.1 模板项目原则

App 层代码应该：

1. 保留；
2. 可编译；
3. 默认不主动运行；
4. 作为用户参考；
5. 不影响底层驱动独立性。

---

### 14.9 Include 路径要求

后续新代码推荐 include 优先级：

```text
App / Libs -> 优先 include 5_facade/*.h
Board      -> include 3_device/*.h
Device     -> include 2_core/*.h、1_platform/*.h、必要 BSP/ti_msp_dl_config.h
Compat     -> include 5_facade/*.h
```

不推荐新代码继续直接依赖旧 `5_compat` 路径。

头文件应尽量最小化 include：

1. 能前置声明就前置声明；
2. `.h` 中只 include 必要类型定义；
3. 具体实现依赖尽量放到 `.c`；
4. 避免在 Core 层头文件 include 具体设备头文件。

---

### 14.10 全局变量与可见性要求

后续代码必须遵守：

#### 14.10.1 实例参数跟着 `me` 走

属于某个实例的参数放入结构体：

```c
struct xxx_s {
    device_base_t base;
    const xxx_ops_t *ops;
    xxx_cfg_t cfg;
    // runtime state
};
```

不要写成全局变量。

#### 14.10.2 模块私有变量使用 `static`

只在本 `.c` 文件中使用的变量：

```c
static uint8_t g_xxx_state;
```

#### 14.10.3 只读配置使用 `static const`

默认配置、查找表、ops 表：

```c
static const xxx_ops_t g_xxx_ops = { ... };
static const xxx_cfg_t g_default_cfg = { ... };
```

#### 14.10.4 不暴露可写全局变量

不要在头文件中写：

```c
extern int xxx;
```

如果确实需要读取状态，提供函数：

```c
int32_t Xxx_GetValue(void);
```

如果确实需要修改状态，提供受控 setter：

```c
void Xxx_SetValue(int32_t value);
```

---

### 14.11 `static` / `inline` 使用要求

#### 14.11.1 文件内函数

只在本文件使用的函数必须为：

```c
static void helper(void);
```

#### 14.11.2 小型工具函数

简单限幅、状态判断、getter 等可为：

```c
static inline int32_t limit_value(...);
```

#### 14.11.3 不要在头文件滥用 inline

除非确实需要 header-only 工具函数，否则不要把大量逻辑写到 `.h`。

---

### 14.12 清理文件要求

后续清理文件时按风险分级。

#### 低风险清理

可以优先删除或移出编译：

```text
5_compat/*.c
```

这些通常只是兼容空壳。

但删除前必须同步修改 Keil 工程文件。

#### 中风险清理

可以考虑删除空的 Facade `.c`：

```text
5_facade/icm42688_facade.c
5_facade/oled_facade.c
5_facade/motor_facade.c
5_facade/stepper_facade.c
5_facade/xunji_facade.c
```

如果未来要补 Facade 逻辑，也可以保留。

#### 高风险清理

暂不建议直接删除：

```text
4_board/*
7_apps/*
5_compat/*.h
```

原因：

- `4_board/*` 中仍有真实板级绑定逻辑（int 文件、ISR 等）；
- `7_apps/*` 是模板示例；
- `5_compat/*.h` 仍承担旧 include 兼容。

---

### 14.13 新增外设时的强制流程

新增外设必须按以下流程：

1. 判断外设类别，确认是否已有合适 ops；
2. 在 `3_device` 中创建对象和 cfg；
3. 将实例参数放入 cfg 或对象结构体；
4. 将板级资源绑定放入 `4_board`；
5. 将业务简化接口放入 `5_facade`；
6. 如需兼容旧路径，在 `5_compat` 添加转发头文件；
7. 不新增可写全局变量；
8. 内部函数全部 `static`；
9. 默认配置和 ops 表使用 `static const`；
10. 完成后做语法检查。

---

## 15. 后续优化建议

当前架构已经完成第一轮迁移，但仍有一些可以继续优化的方向：

1. 将 OLED 的 SCL/SDA GPIO 写函数从 `OLED_int.c` 进一步迁移到 `oled_board.c`。
2. 将 ICM42688 的 I2C 端口和器件地址从 `icm42688_int.h` 进一步迁移到 `icm42688_board.h/.c`。
3. 将 Motor 的 PWM Timer/Channel 默认配置进一步迁移到 `motor_board.c`，让 Device 层更纯粹。
4. 将 Stepper 的 SysConfig 宏、ISR 和双电机状态表进一步拆分到 Board 层。
5. 新业务代码优先 include `5_facade/*.h` 或 `4_board/*.h`，减少对旧 `5_compat` 兼容路径的依赖。
6. 逐步减少全局变量兼容接口，改为函数访问或对象访问。
7. 为每个 Device 层补齐统一风格的 Doxygen 注释。
8. 在确认 Keil 工程引用更新后，删除空壳 `5_compat/*.c`。
9. 需要抽象多个显示后端时，再为 OLED/TFT 拆分 `text_ops`、`graphics_ops` 等专用显示能力接口。

---

## 16. 总结

当前五层架构的核心思想是：

```text
Core 定义通用规则；
Platform/BSP 屏蔽 MCU 底层差异；
Device 实现外设类本体；
Board 绑定当前硬件实例；
Facade/Compat 稳定业务接口并兼容旧代码。
```

这种结构适合嵌入式工程长期演进，尤其适合当前这种外设数量逐渐增加、业务层逐渐复杂、同时又需要兼容历史代码的项目。

最终目标不是为了“分层而分层”，而是为了让每个文件只承担清晰职责，让外设驱动更容易维护、复用、迁移和扩展。
