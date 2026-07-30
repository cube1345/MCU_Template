# STM32F407VE 引脚资源分配

本文记录当前固件对 STM32F407VE 引脚的实际占用和板级预留情况。

- 最后核对日期：2026-07-29
- 硬件目标：STM32F407VE
- 运行入口：`main()` -> `tasks_create()` -> FreeRTOS 调度器
- 主要依据：`empty.ioc`、`layer/board/stm32f407/` 和
  `layer/application/core/app_rtos.c`

## 状态定义

| 状态 | 含义 |
| --- | --- |
| 系统保留 | 时钟、调试或系统时基需要，业务模块不得重新分配 |
| 当前启用 | 默认固件启动后会配置或使用该引脚 |
| 已分配未启用 | Board 层已经确定用途，但默认应用尚未启动对应硬件 |
| 后端候选 | Platform 后端支持该复用方案，但 Board 层没有选择，不算占用 |
| 历史残留 | 未参与当前构建的 CubeMX 旧代码，不算占用 |

## 系统保留引脚

| 引脚 | 系统资源 | 配置 | 状态 | 配置来源 |
| --- | --- | --- | --- | --- |
| PA13 | SWDIO | Serial Wire 调试数据 | 系统保留 | `empty.ioc` |
| PA14 | SWCLK | Serial Wire 调试时钟 | 系统保留 | `empty.ioc` |
| PC14 | LSE_IN | 32.768 kHz 低速外部晶振输入 | 系统保留 | `empty.ioc` |
| PC15 | LSE_OUT | 32.768 kHz 低速外部晶振输出 | 系统保留 | `empty.ioc` |
| PH0 | HSE_IN | 高速外部晶振输入 | 系统保留 | `empty.ioc` |
| PH1 | HSE_OUT | 高速外部晶振输出 | 系统保留 | `empty.ioc` |

TIM14 用作 HAL 系统时基，但不连接外部 GPIO。TIM14 本身仍属于系统保留资源，
不能再分配给普通定时、PWM 或其他业务功能。

## 当前启用引脚

| 引脚 | 用途 | 外设或通道 | 电气模式 | 当前行为 | 配置来源 |
| --- | --- | --- | --- | --- | --- |
| PA0 | USER 按键 | GPIO | 输入，高电平按下 | 10 ms 周期轮询 | `board_key.c` |
| PA1 | AT8236 AIN2（左电机） | TIM2_CH2 | AF1，PWM 输出 | 5 ms 双轴速度闭环 | `board_drive.c` |
| PA2 | AT8236 BIN1（右电机） | TIM2_CH3 | AF1，PWM 输出 | 5 ms 双轴速度闭环 | `board_drive.c` |
| PA3 | AT8236 BIN2（右电机） | TIM2_CH4 | AF1，PWM 输出 | 5 ms 双轴速度闭环 | `board_drive.c` |
| PA5 | AT8236 AIN1（左电机） | TIM2_CH1 | AF1，PWM 输出 | 5 ms 双轴速度闭环 | `board_drive.c` |
| PA6 | 右编码器 A 相 | TIM3_CH1 | AF2，上拉输入 | 5 ms 速度闭环反馈 | `board_drive.c` |
| PA7 | 右编码器 B 相 | TIM3_CH2 | AF2，上拉输入 | 5 ms 速度闭环反馈 | `board_drive.c` |
| PA9 | 日志和上位机 TX | USART1_TX | AF7，推挽 | 115200 baud | `board_uart.c` |
| PA10 | 日志和上位机 RX | USART1_RX | AF7 | 115200 baud | `board_uart.c` |
| PB2 | 蓝色 LED | GPIO | 输出，低电平点亮 | 1000 ms 周期翻转 | `board_led.c` |
| PB6 | OLED SCL | I2C1_SCL | AF4，开漏 | OLED 阻塞通信 | `board_i2c.c` |
| PB7 | OLED SDA | I2C1_SDA | AF4，开漏 | OLED 阻塞通信 | `board_i2c.c` |
| PC5 | 红色 LED | GPIO | 输出，低电平点亮 | 已配置，当前保持熄灭 | `board_led.c` |
| PC6 | EMM V5 TX | USART6_TX | AF8，推挽 | 步进电机低速调速任务 | `board_uart.c` |
| PC7 | EMM V5 RX | USART6_RX | AF8 | 步进电机低速调速任务 | `board_uart.c` |
| PD12 | 左编码器 A 相 | TIM4_CH1 | AF2，上拉输入 | 5 ms 速度闭环反馈 | `board_drive.c` |
| PD13 | 左编码器 B 相 | TIM4_CH2 | AF2，上拉输入 | 5 ms 速度闭环反馈 | `board_drive.c` |
| PD7 | 循迹输入 0（右排针 15） | GPIO | 上拉输入 | 5 ms 采样，不参与控制 | `board_tracking.c` |
| PD5 | 循迹输入 1（右排针 14） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PD3 | 循迹输入 2（右排针 13） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PD1 | 循迹输入 3（右排针 12） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PC12 | 循迹输入 4（右排针 11） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PC10 | 循迹输入 5（右排针 10） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PA12 | 循迹输入 6（右排针 9） | GPIO | 上拉输入 | 5 ms 六路循迹控制 | `board_tracking.c` |
| PA8 | 循迹输入 7（右排针 8） | GPIO | 上拉输入 | 5 ms 采样，不参与控制 | `board_tracking.c` |

说明：即使 PC5 当前没有有效输出，它已经被初始化为
指定用途，仍应视为占用，不能交给其他模块。

## 已分配但默认未启用

| 引脚 | 预定用途 | 外设或通道 | 未启用原因 | 启用入口 |
| --- | --- | --- | --- | --- |
| PB10 | 备用硬件 I2C SCL | I2C2_SCL，AF4 | OLED 已改用 I2C1 | `board_i2c2_init()` |
| PB11 | 备用硬件 I2C SDA | I2C2_SDA，AF4 | OLED 已改用 I2C1 | `board_i2c2_init()` |

这些引脚虽然当前启动后没有配置硬件，但 Board 层已经分配了明确用途。新增功能
需要占用这些引脚时，应先修改或删除原有 Board 配置，而不是直接复用。

## Platform 后端候选映射

以下映射只是 STM32F4 后端具备的能力，当前 Board 配置没有选用，因此不表示占用。

| 候选引脚 | 可选功能 | 当前不能直接选用的原因 |
| --- | --- | --- |
| PB6/PB7 | USART1_TX/RX | 当前已经由 OLED 的 I2C1 占用 |
| PB8/PB9 | I2C1_SCL/SDA | 后端支持，Board 当前未选择 |
| PA8/PC9 | I2C3_SCL/SDA | 后端支持，Board 当前未选择 |

普通 GPIO 后端支持 PA0 至 PE15 的编号空间，但某个引脚可由 GPIO 后端访问，
不代表它在开发板上空闲或适合业务使用。仍需检查本表、芯片数据手册和原理图。

## 已知资源约束

1. PB6/PB7 当前用于 I2C1，不能同时复用为 USART1。
2. TIM3当前整体工作在右编码器模式，PB4上的TIM3_CH1调光PWM已停用，不能同时恢复。
3. TIM2 的 CH1 至 CH4 共享同一个定时器频率，四路 PWM 不能分别设置不同频率；当前也不能再把 TIM2 配置为编码器模式。
4. PC6/PC7 当前用于 USART6。新增 TIM3 等复用映射时必须避让。
5. USART6 当前只创建阻塞 UART 对象；中断接收后端目前只支持 USART1。
6. OLED 当前使用 I2C1 的 PB6/PB7，不再使用 I2C2 的 PB10/PB11。
7. USART6_TX 固定使用 DMA2 Stream6/Channel5；新增 DMA2 功能时必须避让。
8. 右排针 15 至 8 号位已连续分配为八路循迹数字输入，通道顺序不得在应用层重复解释。

## CubeMX 与手写配置边界

`empty.ioc` 当前只管理 HSE、LSE、SWD 和 TIM14。USART、I2C、GPIO、PWM、编码器
等业务资源由 Platform 和 Board 层自行初始化，不依赖 CubeMX 外设初始化。

`Core/Src/usart.c` 和 `Core/Src/gpio.c` 是历史生成文件，未加入当前 Keil 和 CMake
源文件列表，因此其中出现的引脚不能作为当前占用依据。

`Core/Src/stm32f4xx_it.c` 中的 USART1 中断入口属于当前手写集成。重新用 CubeMX
生成代码前必须确认该入口没有被覆盖。

## 维护规则

修改任何板级引脚、外设实例或复用功能时，必须在同一次修改中更新本文档。

1. 先修改 `layer/board/stm32f407/` 中的板级配置，再更新对应表格。
2. 如果修改 HSE、LSE、SWD 或系统时基，同时检查并更新 `empty.ioc`。
3. 只有默认应用确实调用初始化或启动函数时，才把资源列入“当前启用”。
4. Board 已经确定用途但默认应用未启动的资源，列入“已分配但默认未启用”。
5. Platform 映射表新增的合法复用方案只记录为“后端候选”，不能直接写成已占用。
6. 删除模块时同步删除其 Board 分配；仅删除 Application 任务不等于释放 Board 预留。
7. 编译产物、未参与构建的 CubeMX 文件和历史参考代码不能作为占用依据。
8. 提交前搜索 `STM32F4_PIN_`、`tx_pin`、`rx_pin`、`scl_pin`、`sda_pin`、
   `pin_a` 和 `pin_b`，检查是否出现未登记的新分配。

## 核对来源

- `empty.ioc`
- `layer/application/core/app_rtos.c`
- `layer/application/core/app_led.c`
- `layer/application/core/app_key.c`
- `layer/application/core/app_oled.c`
- `layer/application/core/app_motor.c`
- `layer/application/core/app_encoder.c`
- `layer/board/stm32f407/led/board_led.c`
- `layer/board/stm32f407/key/board_key.c`
- `layer/board/stm32f407/i2c/board_i2c.c`
- `layer/board/stm32f407/oled/board_oled.c`
- `layer/board/stm32f407/uart/board_uart.c`
- `layer/board/stm32f407/drive/board_drive.c`
- `layer/board/stm32f407/emm_v5/board_emm_v5.c`
- `layer/board/stm32f407/tracking/board_tracking.c`
