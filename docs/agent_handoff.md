# STM32F407 FreeRTOS OOP 工程接手文档

> 更新日期：2026-07-29  
> 工程路径：`G:\0_my_works\stm32code\template\stm32f407vet6_hal_freertos\oop`  
> 目标芯片：STM32F407VET6  
> 主要工具链：Keil MDK ARMCC 5.06 update 7

## 1. 当前状态摘要

- 当前固件采用自维护的 Platform、Board、Device、Driver、Component 和 Application 分层。
- CubeMX 只保留工程框架和少量系统资源；业务 UART、I2C、PWM、编码器等资源由 Platform/Board 自行初始化。
- FreeRTOS 任务使用 `heap_4` 动态创建，总堆大小为 10 KiB，系统 Tick 为 1 kHz。
- 当前代码已经通过 Keil 编译：`0 Error(s), 0 Warning(s)`。
- 最新 USART1 中断队列接收、`system status` 命令和完整系统统计功能尚无最新烧录实测证据。
- Git 仓库当前没有提交，工程文件基本都处于 untracked 状态。不要依赖 `git diff` 判断文件归属，也不要覆盖现有内容。

## 2. 接手前必须遵守

1. 先阅读 `agent/AGENT.md`，再阅读与任务匹配的 `agent/skills/*/SKILL.md`。
2. 默认使用中文注释，作者统一写为 `weixin`。
3. 函数使用 `snake_case`；公开函数和独立职责函数使用完整的 `@brief/@param/@retval` 注释。
4. HAL 和寄存器访问只能位于 STM32F4 Platform 后端，Application、Component 和 Device 不得直接依赖 HAL 类型。
5. 具体引脚、通道、地址和对象组合放在 Board 层。
6. 修改引脚、外设实例或复用功能时，必须同步维护 `docs/pin_allocation.md`。
7. 只编译不需要硬件授权；烧录、擦除、复位和调试器控制必须获得用户当前任务的明确授权。
8. 不要无任务需求地修正历史拼写，例如 `inferface` 和 `my_freetros_lib.*`，大范围重命名会影响 Keil 与 VSCode 工程清单。

## 3. 分层与依赖方向

```text
layer/application/core       FreeRTOS 任务、功能启用和业务编排
layer/component              多设备或设备+算法组合
layer/device                 稳定设备抽象和公共 API
layer/driver                 AT8236、SSD1306、EMM V5 等具体器件实现
layer/library                PID、滤波、数学、Mahony、单色图形工具
layer/board/stm32f407        板级接线、资源选择、静态对象组合
layer/platform/inferface     与 MCU 无关的平台接口
layer/platform/mcu/stm32f4   STM32F4 HAL/寄存器后端
layer/platform/RTOS          FreeRTOS 内核、移植、堆和监控
```

依赖方向：

```text
application -> board/component/device
component   -> device/library
driver      -> device/platform interface
board       -> driver/platform backend
platform backend -> STM32 HAL/registers
```

不要让依赖反向，也不要让 Application 访问具体 STM32F4 子类。

## 4. API 与对象约定

- `*_bind()`：建立对象、ops 和依赖关系，不访问硬件。
- `*_init()`：具体驱动或 Board 初始化入口，通常完成绑定并启动硬件。
- `*_start()` / `*_stop()`：设备生命周期。
- `*_get()`：获取 Board 单例、基础对象或内部只读数据。
- `*_set_*()`：设置设备状态、输出或控制目标。
- `*_update()` / `*_poll()`：由单一应用任务按固定周期调用。
- 返回值统一为 `int`，状态通过 `PLATFORM_*` 宏表示，成功为 `PLATFORM_OK`。
- ops 表使用 `static const` 和指定初始化器。
- 具体实现的第一个成员通常是基础对象，通过 `&child.base` 向上转型。
- 中等复杂设备使用基础 ops 加固定能力 ops；不支持的能力指针为 `NULL`，公共包装函数返回 `PLATFORM_ERR_UNSUPPORTED`。

## 5. 启动链与活跃任务

实际启动链：

```text
layer/application/core/main.c
  main()
    -> HAL_Init()
    -> tasks_create()
       -> board_uart1_init()
       -> board_drive_bind()
       -> xTaskCreate(...)
    -> rtos_start()
    -> vTaskStartScheduler()
```

`board_drive_bind()` 只组合左右电机、编码器和闭环轴，不启动 PWM 或编码器；真实启动由 `motor_task` 完成。

| 任务名 | 优先级 | 当前职责 |
|---|---:|---|
| `monitor` | 1 | USART1 中断逐字节接收，经 128 项 FreeRTOS 队列解析 WEIXIN 协议 |
| `sys_stats` | 1 | 阻塞等待通知，收到 `system status` 后打印一次系统状态 |
| `led` | 1 | 初始化蓝灯，每 1000 ms 翻转一次 |
| `key` | 2 | 每 10 ms 轮询 USER 按键；当前识别结果未被业务消费 |
| `oled` | 1 | 延时 50 ms 后初始化 OLED，每秒显示递增计数 |
| `motor` | 3 | 左右电机闭环控制，周期 5 ms，从原子邮箱消费目标速度 |
| `emm_v5` | 1 | 延时 2 秒后启动 USART6 步进电机，当前固定运行 60 RPM |
| `tracking` | 1 | 每 5 ms 用中间六路计算差速并发布左右轮目标，200 ms 打印状态 |

未启用或被链接裁剪：

- `app_encoder.c` 已加入 Keil 工程，但没有创建 `encoder_task`。
- `encoder_task` 不能与 `motor_task` 同时启用，因为 `motor_axis` 已独占左右编码器。
- IMU、ICM42688、姿态和 Mahony 模块仍保留，但当前没有 Application 启动路径。
- `Core/Src/freertos.c` 仍参与编译，旧 `MX_FREERTOS_Init()` 未被调用并会被链接裁剪。

## 6. 当前硬件资源

详细信息以 `docs/pin_allocation.md` 为唯一台账，下面是接手快照。

| 资源 | 当前用途 | 状态 |
|---|---|---|
| PA0 | USER 按键，高电平按下 | 已启用，10 ms 轮询 |
| PA1 / PA5 | 左 AT8236 AIN2/AIN1，TIM2_CH2/CH1 | 已启用，10 kHz PWM |
| PA2 / PA3 | 右 AT8236 BIN1/BIN2，TIM2_CH3/CH4 | 已启用，10 kHz PWM |
| PA6 / PA7 | 右编码器 A/B，TIM3_CH1/CH2 | 已启用，硬件编码器模式 |
| PD12 / PD13 | 左编码器 A/B，TIM4_CH1/CH2 | 已启用，硬件编码器模式 |
| PA9 / PA10 | USART1 TX/RX | 已启用，115200，日志和上位机协议 |
| PB2 | 蓝色 LED，低电平点亮 | 已启用，每秒翻转 |
| PC5 | 红色 LED，低电平点亮 | 已初始化，默认熄灭 |
| PB6 / PB7 | OLED I2C1 SCL/SDA | 已启用，400 kHz，SSD1306 地址 0x3C |
| PC6 / PC7 | USART6 TX/RX | 已启用，115200，EMM V5 |
| PB10 / PB11 | 备用 I2C2 SCL/SDA | Board 已分配，默认未启动 |
| PA13 / PA14 | SWDIO/SWCLK | 系统保留 |
| PC14 / PC15 | LSE | CubeMX 系统保留 |
| PH0 / PH1 | HSE | CubeMX 系统保留 |
| TIM14 | HAL 1 ms Tick | 系统保留 |
| DMA2 Stream6/Channel5 | USART6 TX | 已分配 |

资源限制：

- PB6/PB7 已用于 I2C1，不能再切换为 USART1 复用。
- TIM2 四个通道共同驱动左右 AT8236，所有通道共享 PWM 频率。
- TIM3 整体用于右编码器，不能恢复 PB4/TIM3_CH1 LED PWM。
- PC6/PC7 已由 USART6 独占，新增复用功能必须避让。
- PB10/PB11 虽未启动，但属于 Board 预留；复用前先修改 Board 配置和引脚台账。

## 7. 主要功能状态

### 7.1 LED 与按键

- LED 基础操作包含开、关、翻转和可选亮度；当前蓝灯使用 GPIO 实现。
- `key_event` 支持消抖、单击、双击、长按和重复事件。
- 当前 `key_task` 只轮询，尚未将事件接入具体业务。

### 7.2 OLED

- SSD1306 分辨率为 128x64，使用 1 KiB framebuffer。
- 已实现 ASCII、格式化文本、图形和刷新流程。
- 当前通过 I2C1 PB6/PB7 连接；用户已确认该接线能够正常显示。

### 7.3 直流电机与编码器

- AT8236 PWM 公共范围为 `0..10000`。
- 当前闭环输出限幅为 `800`，相当于 8% 最大 PWM；调整前必须确认电机安全。
- 左编码器使用 TIM4，右编码器使用 TIM3，均为硬件正交编码器模式。
- `motor_axis` 组合电机、编码器、速度 PI、位置环和一阶速度低通。
- 当前控制周期为 5 ms（200 Hz）。`motor_task` 独占电机和编码器，并从
  `app_motor` 的临界区邮箱读取一对目标速度；其他任务不直接访问电机对象。
- `drive.left_speed` 和 `drive.right_speed` 是循迹基础速度，默认均为
  `80000 count/s`；循迹任务叠加方向差速后发布最终目标。
- 速度误差死区为 `±300 count/s`；死区内比例误差按零处理并暂停积分，但保留已有积分输出。
- 中间六路 `io1..io6` 的左右位置权重为
  `-5, -3, -1, 1, 3, 5`，输入 `0` 表示检测到黑线。
- 差速增益为每个误差单位 `4000 count/s`。基础速度均为 `80000`
  时，最终目标为 `left=80000+differential`、
  `right=80000-differential`，两轮平均速度保持 `80000 count/s`。
- 六路全黑按路口直行处理；短时丢线保留最后方向，连续丢线达到
  `100 ms` 后发布 `0,0` 停车；启动后尚未检测到线时也保持停车。
- `motor_axis` 连续更新速度目标时保留 PI 积分，仅在切入速度模式时清零。
- 电机方向、编码器方向、PI 参数和高速稳定性仍需要整车实测。

### 7.4 EMM V5

- 使用 USART6 PC6/PC7，波特率 115200。
- USART6 TX 使用 DMA2 Stream6/Channel5，但调用任务仍会等待 DMA 完成。
- 当前 `response_required=false`，只证明帧已发送，不能证明驱动器接受命令。
- 当前任务固定运行 60 RPM，与早期“循环加速再减速”需求不一致，修改前先确认用户当前意图。

### 7.5 I2C

- OLED 当前使用 I2C1 硬件阻塞传输。
- 软件 I2C 后端仍然存在。
- I2C 中断 ops 已存在，但未发现完整的 I2C EV/ER IRQ、NVIC 和 HAL IRQ 接入，不能视为已经可用。

## 8. USART1 与串口协议

USART1 同时承载 `printf` 日志和上位机协议：PA9/PA10，115200 baud。

左右轮实测速度由串口任务以 10 Hz 发送为同一采样点的 WEIXIN 波形：

```text
wave.left_speed=<count/s> wave.right_speed=<count/s>
```

`app_serial_monitor.h` 中的 `SERIAL_MONITOR_DRIVE_DEBUG_ENABLE` 是速度环
串口调试总开关。设为 `1` 时开放左右基础速度、`Kp/Ki` 调参并发送上述
波形；设为 `0` 时四个参数不进入参数表，波形采集和发送代码也不参与编译。
当前默认设为 `0`。固化参数为 `Kp=0.044`、`Ki=0.015`，左右基础速度均为
`80000 count/s`。

接收链：

```text
USART1_IRQHandler
  -> HAL_UART_IRQHandler
  -> 单字节完成回调
  -> xQueueSendFromISR
  -> monitor 任务 xQueueReceive(portMAX_DELAY)
  -> weixin_protocol_feed_byte
```

- ISR 只复制字节、续接下一次接收并投递队列，不解析协议、不打印日志。
- USART1 IRQ 优先级为 5，符合当前 FreeRTOS `FromISR` API 限制。
- 队列溢出或 UART 错误后会丢弃残缺命令直到下一个行结束符。
- 发送仍使用 HAL 阻塞发送；大量日志可能互相交错。

活跃协议配置：ASCII、CRLF、命令前缀 `param`、响应前缀 `[param]`。

```text
param help
param list
param get <name>
param set <name> <value>
system status
```

当前参数（四个 `drive.*` 参数仅在速度环串口调试开关打开时存在）：

| 参数 | 权限 | 范围或含义 |
|---|---|---|
| `system.fw_version` | 只读 | 固件版本编码 |
| `drive.speed_kp` | 读写 | 0..10 |
| `drive.speed_ki` | 读写 | 0..10 |
| `drive.left_speed` | 读写 | 左轮循迹基础速度，-200000..200000 count/s |
| `drive.right_speed` | 读写 | 右轮循迹基础速度，-200000..200000 count/s |
| `monitor.tx_errors` | 只读 | UART 最终发送失败次数 |
| `monitor.rx_errors` | 只读 | UART 接收、队列或续接错误次数 |

GUI 打开串口时，禁止使用 pyserial 或其他串口终端抢占同一个端口。

## 9. FreeRTOS 系统状态监控

- `configUSE_TRACE_FACILITY=1`。
- `configGENERATE_RUN_TIME_STATS=1`。
- 使用 Cortex-M4 DWT `CYCCNT`，不占用通用定时器。
- 使用 64 位软件累计扩展 DWT 的 32 位计数回绕。
- `Show_System_Stats()` 输出任务名、状态、优先级、栈历史最小余量、累计运行时间、`Avg%`、`Win%`、当前空闲堆和历史最小空闲堆。
- 最多统计 24 个任务；超出时整次报告失败并提示。
- `Stack` 单位是 `StackType_t`，当前 Cortex-M4 端口通常为 4 字节，不是字节。
- 第一次 `Win%` 覆盖系统启动至第一次报告；后续才是两次报告之间的窗口。
- `system status` 每发送一次，通过任务通知生成一份报告；没有命令时 `sys_stats` 保持阻塞。
- 报告使用大量阻塞式 `printf`，会扰动当前窗口并可能与其他日志交错。

## 10. 编译、烧录与 VSCode

标准脚本：

```powershell
# 仅编译
powershell -ExecutionPolicy Bypass -File .\tool\build_flash.ps1 -BuildOnly

# 编译并烧录，仅在用户明确授权后执行
powershell -ExecutionPolicy Bypass -File .\tool\build_flash.ps1
```

默认配置：

- Keil：`D:\Keil_v5-537\UV4\UV4.exe`
- 工程：`MDK-ARM/empty.uvprojx`
- Target：`empty`
- 编译器：ARMCC 5.06 update 7
- 输出：`MDK-ARM/empty/empty.hex`
- 日志：`MDK-ARM/empty/codex_build.log`、`codex_flash.log`

脚本通过隐藏窗口调用 UV4，避免弹出 Keil GUI。成功条件必须同时满足：

1. 命令退出码符合脚本约定。
2. 日志包含 `0 Error(s), 0 Warning(s)`。
3. `empty.hex` 存在且来自本次编译。

VSCode 说明：

- Keil 是唯一真实构建链。
- 根目录 `CMakeLists.txt` 只用于生成 IntelliSense 的 `compile_commands.json`。
- 新增或移动源文件时，必须同时维护 Keil 工程和 CMake 源文件清单。
- 跳转异常时运行 VSCode 任务 `CMake: configure VSCode index`，并确认 C/C++ 扩展已启用、工作区受信任。

## 11. 已知风险与待办

1. 最新 UART1 中断队列和 `system status` 只有编译/链接验证，尚未烧录实测。
2. 下一次硬件验证应检查：空闲时 `monitor` 为 `BLK`，`Win%` 接近 0，每条 `system status` 只产生一份报告。
3. `serial_monitor_init()` 附近仍可能存在“阻塞接收”历史注释，需要在后续相关修改中同步修正。
4. `tasks_create()` 中途失败不会清理已经创建的任务；当前主流程失败后直接进入 `Error_Handler()`，但未来若增加重试必须补回滚。
5. `printf`、USART1 协议响应和系统状态报告共享串口，输出可能交错，尚无统一 TX 互斥或日志队列。
6. I2C 中断能力接口不等于硬件链路已实现，使用前必须补齐 IRQ 和实测。
7. 实际运行时钟代码使用 HSI + PLL 168 MHz，而 `empty.ioc` 中仍包含 HSE/LSE 配置。修改时钟前先统一设计意图。
8. 电机闭环输出限幅为 800/1000，对应 PWM 满量程的 80%；运行前必须确认供电、机械负载和急停条件。
9. USART6 EMM V5 当前不校验设备应答。
10. 不要同时启用独立编码器任务和闭环电机任务。

## 12. 下一位 Agent 的建议起步顺序

1. 阅读 `agent/AGENT.md` 和当前任务对应的 Skill。
2. 阅读 `docs/pin_allocation.md`，确认资源没有冲突。
3. 阅读 `layer/application/core/app_rtos.c`，确认当前真实启动任务。
4. 阅读目标功能对应的 Board 文件，确认具体对象、引脚和初始化责任。
5. 使用 `tool/build_flash.ps1 -BuildOnly` 建立干净编译基线。
6. 只有获得用户明确授权后才烧录。
7. 首次烧录优先验证 USART1 中断接收和 `system status`。
8. 再根据任务范围修改代码，不进行无关重命名或目录整理。

## 13. 关键文件索引

| 文件 | 作用 |
|---|---|
| `agent/AGENT.md` | 工程最高级本地规范 |
| `docs/pin_allocation.md` | 引脚和外设资源唯一台账 |
| `layer/application/core/main.c` | 实际程序入口 |
| `layer/application/core/app_rtos.c` | 活跃任务和启动配置 |
| `layer/application/core/app_serial_monitor.c` | USART1 队列接收、参数表、自定义命令 |
| `layer/application/core/app_system_stats.c` | 按需系统状态报告 |
| `layer/application/core/app_motor.c` | 当前双电机闭环业务 |
| `layer/application/core/app_tracking.c` | 中间六路循迹差速计算与目标发布任务 |
| `layer/component/motor_axis/` | 电机、编码器、PI 和滤波组合 |
| `layer/component/tracking/` | 八路循迹数字输入采样组件 |
| `layer/board/stm32f407/drive/board_drive.c` | 双电机和编码器资源绑定 |
| `layer/board/stm32f407/uart/board_uart.c` | USART1/USART6 配置 |
| `layer/platform/mcu/stm32f4/uart/stm32f4_uart.c` | UART、IRQ 和 DMA 后端 |
| `layer/platform/mcu/stm32f4/i2c/stm32f4_i2c.c` | I2C HAL 后端 |
| `layer/platform/RTOS/FreeRTOSConfig.h` | FreeRTOS 核心配置 |
| `layer/platform/RTOS/monitor/my_freetros_lib.c` | DWT、任务和堆统计 |
| `layer/component/weixin_protocol/` | WEIXIN 文本协议组件 |
| `tool/serialmonitor/runtime/protocol.json` | 当前 GUI 协议配置 |
| `tool/build_flash.ps1` | 标准编译烧录脚本 |
| `MDK-ARM/empty.uvprojx` | 真实 Keil 工程 |
| `MDK-ARM/empty/empty.map` | 判断代码是否进入最终固件 |
