# 项目状态

更新日期：2026-07-29

## 当前结论

模板已具备 `HAL → Core → Device → Board → Facade → App` 的五层骨架，并已有
静态 ops 表、Board 配置注入和 Facade 兼容入口。下一轮不重建目录；先消除构建
配置漂移和已确认的反向依赖，再以单个驱动作为可复制样板。

## 已验证基线

- 分支：`main`；优化开始前工作树干净。
- 增量 Keil 构建可掩盖未重新编译的源文件；clean rebuild 是后续提交的必要检查。
- `empty.syscfg` 配置为 MSPM0G3507、LQFP-64 与 40 MHz 外部晶振；这些是配置假设，
  尚未连接硬件确认。
- 根 `CMakeLists.txt` 仍引用迁移前的 `MyUsed/*` 路径，且会递归收集整棵 TI SDK；
  这是当前最高优先级的构建风险。
- `2_device/motor_driver.c` 仍直接依赖 Board/DriverLib；它将作为边界收敛样板。

## 已知限制

- 当前未连接硬件，不能验证 PWM 波形、GPIO 极性、I2C 时序、IRQ 和电机方向。
- `6_apps/imu_app.c` 依赖缺失的 `imu_int.h`，不应在修复前加入受支持构建清单。
- UART ISR 与云台 legacy 测试仍有跨层和并发风险；先记录，后续独立重构，避免与
  Motor 改动混合。

## 近期路线

1. 使 Keil/CMake 只依赖仓库内的实际层级源文件。
2. 使用 PWM port 的 Strategy/Adapter 注入收敛 Motor 的 Device 边界。
3. 为新公共 API 添加状态契约、Doxygen 和 host-test 入口。
4. 以硬件 smoke test 验证 PWM、I2C、Encoder、Stepper 和 UART。

## 变更记录

- 2026-07-29：建立本状态文档、项目内 build/refactor skill 与构建治理 ADR。
