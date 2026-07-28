# 外部资料审阅与 MCU_Template 架构建议

> 审阅日期：2026-07-29
> 范围：`E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master`；仅审阅与建议，未改动项目源代码。

## 结论

当前项目的 `0_hal → 1_core → 2_device → 3_board → 4_facade → 6_apps`
分层方向正确，且 LED、ICM42688、MPU6050、Stepper 已经采用了对象配置和
Board 注入，具备继续演进为可维护模板的基础。下一阶段不宜重写五层架构，
而应先封住已确认的跨层路径、统一错误/生命周期契约、收敛重复兼容 API，
再按真实需求引入小粒度的设计模式。

**重要原则：23 种 GoF 模式不是完成清单。** 外部资料实际推荐的是状态、观察者、
策略、工厂、单例、适配器和表驱动等模式；仅在它能消除已存在的变化点或重复时使用。
在资源受限的 MCU 上，避免为了“纯 OOP”增加堆分配、全局注册表或多余的动态分派。

## 已核实的当前基线

| 项目项 | 已验证事实 | 判断 |
| --- | --- | --- |
| 五层设计 | `docs/myused_five_layer_architecture.md` 已定义 HAL/Core/Device/Board/Facade，Keil 工程包含 `0_hal` 至 `6_apps`。 | 保留并强化，而非目录级重构。 |
| 已有依赖注入 | `icm42688_cfg_t.i2c`、`mpu6050_cfg_t.i2c`、OLED 的 SCL/SDA 回调、Stepper 的 `stepper_hw_cfg_t` 都将板级资源注入 Device。 | 这是后续驱动的首选范式。 |
| 已有 OOP 骨架 | `device_base_t`、各类 `device_*_ops_t` 与 Device 内的 `static const` ops 表已经存在。 | 可演进，但要避免万能 `void *` 接口泛化。 |
| Keil 主构建 | `keil/MSPG3507_FreeRTOS.uvprojx` 使用 `ARM-ADS`，并显式列出分层源码。 | 任何 section/initcall 设计必须先验证 ARM-ADS 与 scatter 文件行为。 |
| 构建配置漂移 | 根 `CMakeLists.txt` 仍指向不存在的 `MyUsed/*`，同时递归加入全部 `source/*.c` 与根 `*.c`。 | 高优先级修复；CMake 不能代表当前 Keil 项目。 |

## 外部资料中可直接采用的原则

| 优先级 | 资料与具体路径 | 可采用的做法 | 对本项目的落点 |
| --- | --- | --- | --- |
| P0 | `ai-coding-skill/references/architecture.md` | 应用只依赖稳定服务；Device 只依赖抽象；仅最底层接触厂商库。 | 把剩余的 `DL_*` 调用赶回 HAL/Board；清除 Device→Board 的反向包含。 |
| P0 | `ai-coding-skill/references/hardware-interaction.md` | ISR 极简；ISR 与任务通过安全事件/队列/标志同步；先验证 API 是否阻塞。 | UART 接收改为“ISR 入队/通知、任务解析或受控裸机轮询”；为共享数据定义一致性边界。 |
| P1 | `ai-coding-skill/references/design-patterns.md` | 使用状态、策略、观察者、适配器、工厂、表驱动解决明确的变化点。 | 保留已有 ops/Adapter；把协议和控制模式从巨型 `switch`/全局状态中逐步抽离。 |
| P1 | `oop-in-c/code/15-platform/platform/platform_i2c.h` | I2C 分为 bus（控制器/锁/传输）与 client（总线/地址），驱动持有 client。 | 将两套 IMU 的相近 `i2c_ops` 收敛为通用 bus/client 契约，设备地址放在 client/config。 |
| P1 | `oop-in-c/code/15-platform/drivers/led/led_base.h` | 父类做统一分派和状态，ops 明确标记必填/可选；实例共享 `static const` ops。 | 每类 Device 声明能力契约、返回统一状态码，避免静默跳过空 ops。 |
| P1 | `ai-coding-skill/references/clean-code.md` | 失败必须传播；外部输入运行时校验；内部不变量用断言；表驱动替代重复分支。 | 引入小型项目级 `mcu_status_t`；初始化/硬件调用不再只 `void` 返回。 |
| P1 | `ai-coding-skill/references/memory-safety.md` | ISR/主循环共享状态需原子性或临界区；缓冲区携带长度；避免隐式可变全局。 | UART 解析结果采用快照或 SPSC ring buffer；明确 `volatile` 只适用于标志而不保证结构体一致性。 |
| P2 | `oop-in-c/code/17-initcall/pc/initcall.h` | 模块自注册与分级初始化可降低主函数维护成本。 | 仅作为“可行性 spike”；须为 ARM-ADS/`.sct` 编写并验证专用 section，不能照搬 GCC `__start/__stop`。 |
| P2 | `ai-coding-skill/references/code-style.md` | `static` 限制可见性、`const` 表示只读、Doxygen 解释边界与上下文。 | 新/改公共 API 补充 Doxygen；统一 include guard；不做无验证的大规模改名。 |

## 已确认的优先修复项

### P0 — 封住违反层级的硬件访问

1. `2_device/motor_driver.c` 包含 `../3_board/motor_board.h`，并直接调用
   `DL_Timer_*`。这与当前文档“Device 只依赖 Core/HAL、板级资源由 Board 注入”
   相冲突，也使电机逻辑难以 host test。
2. `6_apps/yuntai_app.c` 存在直接 `DL_GPIO_*` 调用；应用层不应配置 STEP
   GPIO/复用功能。该测试/校准路径应成为 Stepper Device 的受控能力，实际管脚
   切换放到 Board/HAL。
3. `6_apps/usart_app.c` 的 ISR 使用 `DL_UART_MAIN_IIDX_RX` 枚举并在中断中执行
   协议解析、写入多字段 `g_usart_result`；主循环可能同时读该对象。8 位标志的
   `volatile` 不能保证整个结果结构的一致快照。

建议的最小路径：

```text
App ──Facade/Service──> Device API ──> injected HW port ──> Board ──> HAL ──> DriverLib
                                  ^
                       仅包含 Core 定义的窄 ops/配置
```

- 为 Motor 定义窄的 PWM 端口（例如 `start/stop/set_compare`），由 Board 填充，
  Device 仅处理限幅、死区和 PI；优先复用并补齐 `hal_timer`，不要让 Device 引入
  `ti_msp_dl_config.h` 或 Board 头文件。
- UART ISR 只读取 FIFO 到固定大小 SPSC ring buffer 或投递通知；解析和结果更新
  放在主循环/任务上下文。若暂时保持 ISR 解析，至少以临界区复制结果、记录溢出，
  并在 Doxygen 中说明 ISR 上下文与并发契约。
- 把 HAL 的中断状态从厂商枚举映射为项目枚举，应用层不再知道 `DL_*`。

### P0 — 恢复唯一可信的构建描述

根 `CMakeLists.txt` 引用的 `MyUsed/MyApps`、`MyUsed/MyCore` 等目录已不存在；
`file(GLOB_RECURSE ...)` 还会把整棵 TI SDK 的示例/库源码纳入目标。应以 Keil
工程的实际源清单为基线建立一个显式 CMake target（或明确标记 CMake 仅用于索引）。

- include path 改为真实的 `0_hal`、`1_core`、`2_device`、`3_board`、
  `4_facade`、`5_libs`、`6_apps`；
- 源文件使用显式列表或受限目录列表，绝不递归编译整个 `source/`；
- 将 host test target 与 firmware target 分开，避免 host 编译器接触 MCU startup、
  DriverLib 与 FreeRTOS port；
- 每次增删源码同时更新 `.uvprojx` 和 CMake，或由单一清单生成另一方。

Keil 的 include path 还保留了缺失的 `..\\msp_freertos_module`，以及项目外的
`..\\2026secondrace`。本次不删除项目外文件；后续应先确认它们是否为真实依赖，
再清理这两条引用。

### P1 — 统一对象生命周期、错误与可见性

当前大量 `Init`/设备方法返回 `void`，在空指针、未初始化、HAL 失败时静默返回，
调用者无法区分成功与失败。例如 `LED_Init`、`Motor_InitDevice`、`Buzzer_InitDevice`
等均如此。建议先定义一个轻量级、无动态分配的状态码类型：

```c
typedef enum {
    MCU_STATUS_OK = 0,
    MCU_STATUS_EINVAL,
    MCU_STATUS_ESTATE,
    MCU_STATUS_EIO,
    MCU_STATUS_EBUSY,
    MCU_STATUS_ENOTSUP,
} mcu_status_t;
```

新 API 先使用该约定；兼容 Facade 可暂时保留 `void` 包装并按项目策略记录/上报
失败。不要一次性改变所有公开函数签名。每个对象应有清晰状态：`UNINIT`、`READY`、
`RUNNING`、`FAULT`（按需要选择），并指定重复初始化、停止、析构/复位的行为。

### P1 — 去重时先收敛入口，后删除实现

`motor_driver.h/.c` 同时维护对象 API（`Motor_*Device`、`Motor_*`）和历史全局 API
（`Set_PWM`、`Velocity_A/B`、`limit_PWM` 等），容易继续分叉。Facade 的职责正是保存
稳定入口，因此建议：

1. 以类型化的对象 API 作为唯一真实实现；
2. 将历史函数移至对应 `*_facade.c`，仅作薄转发并以 Doxygen 标记为兼容接口；
3. 用 `git grep` 确认调用点为零、Keil 编译成功后才删除旧入口；
4. 每个外设单独提交，禁止“删除一批未验证的旧代码”。

`device_ops.h` 也应避免继续新增大而全的 `void *` ops。通用性只到能力边界：
GPIO、PWM、I2C bus、显示文本/绘图、传感器采样等应是聚焦接口；如果仅被一个具体
驱动使用，就定义在该驱动私有头或 Device 内部。这样保留 C OOP 的多态收益，同时
避免 Core 成为新的万能层。

## 设计模式选型（按已知需求）

| 模式 | 当前适用点 | 建议 | 不适用/注意 |
| --- | --- | --- | --- |
| Adapter | `hal_gpio`、`hal_uart`、`hal_timer` 已经是 TI DriverLib 适配器。 | 继续将厂商枚举、寄存器类型收束到 HAL。 | HAL 不要再混入业务语义。 |
| Strategy / ops | I2C 传输、PWM 实现、IMU 融合/控制算法。 | 用 `static const` ops + 配置注入，契约含错误码、阻塞性、ISR 安全性。 | 不要让每个简单 GPIO 都多一层虚表。 |
| State | UART 帧解析、Stepper/云台控制状态。 | 把状态和事件定义为枚举/表，明确 entry/exit/超时。 | 状态少时保持扁平 FSM，不强上 HSM。 |
| Observer | Key、编码器、UART 完成/故障通知。 | 采用固定容量订阅表或事件队列；回调带 `ctx`。 | ISR 不直接广播/加锁；先投递事件。 |
| Command + 表驱动 | UART 命令到处理器、错误码文本、配置校验。 | 使用 `const` 映射表减少增长型 `switch`。 | 表项校验冲突、覆盖及默认错误码。 |
| Factory | 同类但可替换的传感器/显示后端。 | 仅在运行时根据板型/ID 选择实现时引入。 | 静态单板模板无需动态工厂。 |
| Singleton | 板级时钟、唯一 UART bus、系统服务。 | 使用静态实例 + 显式启动期初始化。 | 不把所有全局变量包装成“单例”。 |
| Initcall | 模块自注册与分级初始化。 | 仅在 ARM-ADS 专用链接段验证通过后引入。 | GCC 示例的 `__attribute__`、`__start/__stop` 不可直接复制。 |

## 推荐目标架构

```text
6_apps          业务状态机/策略；不含 DriverLib、寄存器或具体管脚
4_facade        稳定且小的应用入口；兼容旧 API 的唯一位置
5_libs          无硬件依赖的算法、协议、容器（或按职责下沉/上移）
2_device        可复用设备状态机、算法、协议；依赖 Core 的窄接口
3_board         本板实例、SysConfig 宏、地址/引脚/IRQ 绑定、ISR 最小入口
1_core          status、断言、窄硬件端口契约、时间/事件基础类型
0_hal           MSPM0 DriverLib Adapter；唯一直接调用 `DL_*` 的实现层
SDK/生成配置    根目录 `ti_msp_dl_config.*` 与 `source/` 下 TI/CMSIS/FreeRTOS；视为外部边界
```

目录名称可暂时不动，以免影响 Keil；重点是依赖方向。`5_libs` 不能调用 Board/HAL，
若含 OLED 菜单等业务逻辑，应由 App 依赖它，而不是反向依赖。Board 中允许出现
SysConfig 宏和 IRQ 入口；Device 中不应出现具体 `*_INST`、引脚宏、`DL_*` 函数调用。

## 实施顺序与原子提交建议

1. **构建基线**：记录 Keil clean build、修正 CMake 的实际目录/源清单；提交
   `build: align CMake target with layered firmware sources`。
2. **边界收口**：补全 timer/PWM HAL，迁移 Motor 的 DriverLib 与 Board 依赖；提交
   `refactor(motor): inject board PWM port into device layer`。
3. **并发收口**：重构 UART ISR 与主循环数据交接，加入溢出/错误可观测性；提交
   `refactor(uart): isolate ISR reception from frame parsing`。
4. **契约基础**：新增 `mcu_status_t`、对象状态和 Doxygen 模板，只迁移一个低风险
   Device（LED 或 Buzzer）作为样板；提交
   `feat(core): add firmware status and lifecycle contract`。
5. **兼容收敛**：每个已验证的设备将历史 API 迁至 Facade；删除调用点为零的重复实现；
   每个外设一条 `refactor(<device>): consolidate legacy facade wrappers`。
6. **测试资产**：利用注入的 GPIO/I2C/PWM mock 对 LED、Motor 限幅、UART FSM、IMU
   数据转换做 host 单元测试；提交 `test(<module>): add host coverage for injected port`。
7. **可选探索**：为 ARM-ADS 的 `.sct` 验证分级初始化 section；单独试验提交，失败
   可直接回退，不与产品重构混合。

每一项提交前执行：查看 `git diff --check`、确认 Keil 工程源清单、运行相应编译；
涉及 ISR/共享状态再按外部资料的内存安全与硬件检查清单复核。当前未连接硬件，构建
通过不等于时序、GPIO 极性、I2C ACK、PWM 波形已验证，这些应保留为板上 smoke test。

## 资料索引

- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\architecture.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\design-patterns.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\clean-code.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\memory-safety.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\hardware-interaction.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\ai-coding-skill\references\safety-checklist.md`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\oop-in-c\code\15-platform\platform\platform_i2c.h`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\oop-in-c\code\15-platform\drivers\led\led_base.h`
- `E:\EmbeddedDevelopment\Resource\zhaoming_embedded-master\zhaoming_embedded-master\oop-in-c\code\17-initcall\pc\initcall.h`

## 本次核验边界

- 未修改项目 C/H、Keil、CMake 或生成配置；仅新增本审阅文档。
- 未删除任何项目内、项目外文件。
- 未把参考项目的 GCC/Linux 示例直接套入 ARM-ADS/MSPM0 工程；涉及链接段、RTOS、
  时序和硬件寄存器的建议均标注为后续验证项。
