# ADR-0001：以 Keil 为固件构建权威，CMake 使用受控源清单

## 状态

Accepted

## 日期

2026-07-29

## 背景

工程实际由 ARM-ADS/Keil 编译，但 CMake 仍引用已删除的 `MyUsed/*` 目录并递归
收集整个 TI SDK。这会将 SDK 示例、多个 `main` 和无关组件纳入同一 target。Keil
include path 也保留了仓库外、已不存在的目录，构建依赖无法从仓库复现。

## 决策

- Keil `MSPG3507_FreeRTOS.uvprojx` 是固件二进制的权威构建入口。
- CMake 只列入该模板需要的 DriverLib、FreeRTOS、生成配置和实际五层源目录；不递归
  收集 `source/`。
- Keil 与 CMake 都从仓库根目录解析 `ti_msp_dl_config.h`，不依赖相邻工程目录。
- `empty.syscfg` 是生成源；默认使用已提交输出。重新生成必须显式设置
  `SYSCONFIG_REGENERATE=1`，默认脚本不烧录硬件。

## 后果

- 新增或删除受编译源码时，必须同步 Keil 与 CMake 清单。
- CMake configure 用于清单和编辑器验证；最终 MCU 二进制仍由 Keil clean rebuild 验证。
- SysConfig、PWM、I2C 与中断配置的功能正确性仍需要硬件 smoke test。
