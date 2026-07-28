# MCU 模板架构与重复代码审计

日期：2026-07-29
范围：仅审计 `0_hal` 至 `6_apps`、CMake、Keil 工程与项目架构文档；未修改既有源码、未运行编译或硬件测试。

## 已验证的结论

| 优先级 | 发现与证据 | 影响 | 建议 |
| --- | --- | --- | --- |
| P0 | `CMakeLists.txt` 仍包含不存在的 `MyUsed/*` include 路径，并以 `file(GLOB_RECURSE ... "source/*.c" ... "*.c")` 收集源码。仓库 `source/` 有 556 个 C 文件，且 `source/ti/sasi_smoke_detector/app/main.c` 与 `empty.c` 都定义 `main`。 | CMake 目标会把完整 TI SDK 的示例、无关中间件和产品代码纳入模板，至少存在多入口、符号冲突和不可复现构建风险。 | 以显式 source manifest 替换全局 GLOB；只列入本模板所需的 DriverLib、FreeRTOS、SysConfig 与 `0_hal`~`6_apps`。删除失效的 `MyUsed` 路径。 |
| P1 | `6_apps/imu_app.c` 是五层业务代码中唯一未加入 `keil/MSPG3507_FreeRTOS.uvprojx` 的 C 文件；它 `#include "imu_int.h"`，而仓库内不存在此文件，且有 3 个 `imu_int_*` 调用。 | CMake 修复后会直接失败；Keil 当前未编译它而掩盖了问题，模板示例不具备“可编译”承诺。 | 在二选一决策前不要加入 Keil：要么恢复明确归属的 `imu_int` 边界，要么改为调用现有 ICM/IMU Facade；随后把该文件纳入两套构建入口并编译验证。 |
| P1 | Device 向 Board 反向依赖：`2_device/motor_driver.c` include `../3_board/motor_board.h`，`2_device/xunji_driver.c` include `../3_board/xunji_board.h`。前者还直接调用 `DL_Timer_*`；`stepper_driver.c` 直接配置 NVIC。 | 违反已记录的依赖方向（Device 不应知道板实例/中断控制），使复用和板级迁移失效。 | 将默认单例、历史 API（如 `Motor_Init`/`Set_PWM`、`Xunji_Get`）迁至对应 Facade；Board 直接构造 cfg 并调用 `*_InitDevice`。将 PWM/中断启动收敛为 HAL/Board 责任。 |
| P1 | `6_apps/yuntai_app.c` 在 1269 行后重新配置并直接驱动 Step 引脚（`DL_GPIO_*`），同时公开 7 个 `Yuntai_Test_*` API；它与 `2_device/stepper_driver.c`、`3_board/stepper_board.c` 共同控制同一类资源。 | App 可绕过 Device/Board 状态机，调试或方向校准时可能改变 PWM 引脚复用，形成双控制源。 | 先将 legacy test/校准流程放到受控 feature flag 或 `examples/`，再用 Stepper 的稳定 API 逐步替换直接 GPIO 脉冲；这是需要硬件回归的中风险重构，不能直接删除。 |
| P2 | `3_board/icm42688_board.c` 和 `3_board/mpu6050_board.c` 均为 106 行；两者的 7 个 I2C 回调、端口/地址绑定和 ops 表结构相同，仅传感器类型、地址与默认 init cfg 不同。 | 同一总线适配逻辑会随传感器数量线性复制，修改错误处理时容易漂移。 | 保留类型安全的 sensor 配置，提取 `board_i2c_reg8_adapter`（Adapter 模式）或一个私有工厂；先用单元/编译检查锁定现有读写行为，再做机械替换。 |
| P2 | `5_libs/my_system_lib.c` 直接使用 `DL_TimerA_*`，而架构文档宣称 DriverLib 调用应止于 HAL；`icm42688_driver.h` 暴露 9 个可写 debug 全局，`IMU.h` 暴露多组姿态/debug 全局。 | 平台时间服务与业务库耦合；调试变量成为不受控公共写接口，ISR/任务并发语义不清晰。 | 新建 `hal_systimer` 或 Board time service，并提供 `My_GetTick/My_GetTimeUs` 的稳定接口；debug 状态改为只读 snapshot getter，或置于显式 `DEBUG` 编译开关。 |
| P3 | `build/CMakeCache.txt`、`build/CMakeFiles/*` 被 Git 跟踪，而 `.gitignore` 不忽略根目录 `build/`。 | 主机、工具链与绝对路径相关的生成状态会污染提交和审查。 | 在独立小提交中移除已跟踪的 CMake 生成物，并加入 `/build/` 忽略规则；不删除任何 vendor source。 |

## 当前分层评价

`device_base_t` + ops 表、Board cfg 注入、Facade 兼容入口已经形成了正确的骨架：其中 ICM42688、MPU6050、OLED、MT6701 的注入方式尤其符合 Strategy/Adapter/Facade 的组合。

现有问题不是“抽象不足”，而是迁移期兼容 API 与测试代码仍留在错误层。下一轮应优先收紧依赖方向和构建边界，避免为了套用 23 种模式再增加无业务价值的类型、函数指针或文件。函数指针只保留在可替换总线、设备 ops 和可测试回调等真实变化点。

## 建议的低风险提交序列

1. `build(cmake): restrict template source manifest`：修正 CMake include/source 清单，清理跟踪的 CMake 缓存；不改变 Keil 行为。
2. `refactor(motor-xunji): move default-instance shims to facade`：移除 Device→Board include，将旧函数迁入 Facade，保持函数签名和调用行为。
3. `refactor(platform): move timer and irq ownership to hal-board`：补齐通用 PWM/系统 tick HAL 包装，把 Device 中的 `DL_*`/NVIC 调用迁至 Board；先做编译、再做硬件 PWM/编码器/步进 smoke test。
4. `fix(imu-app): restore a buildable integration boundary`：解决不存在的 `imu_int.h`，同步 CMake 与 Keil source list，并以 clean Keil build 验证。

I2C adapter 去重和 Yuntai legacy test 收敛应在以上四项稳定后单独提交；前者需覆盖寄存器读写错误路径，后者需实际硬件验证方向、脉冲和 PWM 引脚复用。

## 验收标准

- CMake 和 Keil 使用同一份受控模块清单；CMake target 不再包含 TI 示例中的第二个 `main`。
- `0_hal -> 1_core -> 2_device -> 3_board -> 4_facade -> 5_libs/6_apps` 不存在低层指向高层的 include；仅 Board/生成代码持有引脚、实例和 NVIC 细节。
- 所有 `6_apps` 示例要么进入 Keil/CMake 并成功编译，要么被明确标为不随模板构建的 example。
- 公共 API、ISR、回调和硬件假设补充 Doxygen；debug/兼容 API 有生命周期与并发说明。
- 变更后至少执行 clean Keil 编译、CMake 配置/编译和硬件 smoke test（PWM、I2C、Encoder、Stepper）。当前未连接硬件，最后一项尚不能验证。

## 未触碰的内容

- 未删除 `source/` 下的 TI vendor SDK；尚未证明各组件均无构建依赖。
- 未对寄存器、时钟、引脚或中断优先级作判断；本审计未查阅芯片手册、板卡原理图和 errata。
- 未修改已有工作区变更：`build_log.txt` 在审计期间已处于修改状态。
