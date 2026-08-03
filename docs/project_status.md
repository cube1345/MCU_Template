# 项目状态

更新日期：2026-08-03

## 当前结论

模板已具备 `HAL → Core → Device → Board → Facade → App` 的五层骨架，并已有
静态 ops 表、Board 配置注入和 Facade 兼容入口。Motor 已成为首个 PWM port
Strategy/Adapter 注入样板；下一轮继续按同一原则收敛剩余反向依赖。

IIC HAL 已改为接收显式 `I2C_Regs *` 实例；SysConfig 实例宏、总线速率和 IRQ
绑定集中在 `3_board/iic_board.c`，HAL 可脱离当前板级生成宏编译。
OLED、ICM42688 和 MPU6050 已统一通过 `1_core/device_iic.h` 的 `device_iic_t`
组合 endpoint 注入总线；硬件 IIC 与 OLED 软件 IIC 分别由 Board transport 实现。

## 已验证基线

- 分支：`main`；优化开始前工作树干净。
- 增量 Keil 构建可掩盖未重新编译的源文件；clean rebuild 是后续提交的必要检查。
- `empty.syscfg` 配置为 MSPM0G3507、LQFP-64 与 40 MHz 外部晶振；这些是配置假设，
  尚未连接硬件确认。
- CMake 已限定为模板的 DriverLib、FreeRTOS、五层代码和已被 Keil 编译的 App 示例；
  Ninja configure 已通过。
- Motor 的 Device 层已不依赖 Board、SysConfig 或 DriverLib；Timer、通道与极性通过
  Board 的静态 PWM port 注入，兼容 API 已移动至 Facade。
- ARMClang 已对 Motor 的 Device、Board 与 Facade 文件完成语法检查。
- MinGW host 语法检查已通过 `hal_iic.c`（IRQ、轮询及强制轮询寄存器读配置）、
  `iic_board.c`、ICM42688/MPU6050 Board 文件；CMSIS 的 32 位指针警告属于 host
  检查环境限制。

## 已知限制

- 当前未连接硬件，不能验证 PWM 波形、GPIO 极性、I2C 时序、IRQ 和电机方向。
- `6_apps/imu_app.c` 依赖缺失的 `imu_int.h`，不应在修复前加入受支持构建清单。
- UART ISR 与云台 legacy 测试仍有跨层和并发风险；先记录，后续独立重构，避免与
  Motor 改动混合。
- Keil/CMake 的 IIC 绑定现在由 `3_board/iic_board.c` 提供；未配置 IIC 实例时，
  Board 访问器返回空指针，HAL 返回参数错误。
- 当前 UV4 CLI 会启动驻留 GUI 进程但不写入新日志；在此工具行为恢复前，完整 Keil
  clean rebuild 不能作为本轮已通过的验证项。`run_keil.bat` 已在启动前清除旧日志，
  因此不会将历史构建结果误判为通过。

## 近期路线

1. 恢复并验证可重复的 Keil clean rebuild。
2. 为新公共 API 添加状态契约、Doxygen 和 host-test 入口。
3. 收敛 UART ISR/任务边界与 Xunji 的反向依赖。
4. 以硬件 smoke test 验证 PWM、I2C、Encoder、Stepper 和 UART。

## 变更记录

- 2026-07-29：建立本状态文档、项目内 build/refactor skill 与构建治理 ADR。
- 2026-07-29：Motor 改用 Board 注入 PWM port；ARMClang 语法检查通过，硬件验证待执行。
