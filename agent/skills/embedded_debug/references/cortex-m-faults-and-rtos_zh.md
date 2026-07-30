# Cortex-M Fault 与 RTOS 分诊

英文版本：[cortex-m-faults-and-rtos.md](cortex-m-faults-and-rtos.md)。

仅用于已经确认的 Cortex-M 目标。不同内核支持的寄存器不同，请先确认架构，再请求
Fault 寄存器。

## 核心快照含义

- `PC`：采样时的指令地址。
- `LR`：返回地址或异常返回编码。
- `SP`、`MSP`、`PSP`：当前和候选栈位置。
- `xPSR`：执行状态和当前异常号。
- `PRIMASK`：全局可配置中断屏蔽。
- `BASEPRI`：实现该功能的内核上的优先级阈值屏蔽。
- `FAULTMASK`：实现该功能的内核上的 Fault 屏蔽。

使用生成当前运行固件的精确 ELF/AXF/map 符号化 PC/LR。把 LR 当作普通代码地址前，
先检查它是否为 `EXC_RETURN`。

## Fault 证据

在内核支持时，检查 CFSR、HFSR、SHCSR、MMFAR、BFAR、DFSR 和 AFSR 等 SCB Fault
状态。根据架构和器件文档确定地址和含义；Cortex-M0/M0+ 与 M3/M4/M7/M33 类目标
提供的可配置 Fault 集不同。

复位前采集异常栈帧。根据异常返回状态判断使用的是 MSP 还是 PSP，并结合当前链接 map
和 RAM 范围验证栈地址。

## FreeRTOS 与 BASEPRI

Cortex-M 端口的 FreeRTOS 临界区通常使用 `BASEPRI` 屏蔽达到 syscall 允许紧急程度
及以下的中断。具体编码值取决于已实现的优先级位数和项目配置。

排查 `HAL_Delay()` 或其他 tick 等待卡死时：

1. 在安全可读时读取 PC、BASEPRI、PRIMASK 和 tick 值。
2. 确认真实 HAL timebase 来源及其中断优先级。
3. 读取当前项目的 FreeRTOS syscall 中断阈值。
4. 判断观察到的临界区是否屏蔽了 timebase 中断。
5. 修改优先级前检查 timebase ISR 实现。

按常见 CMSIS 数值约定，数字越小，硬件紧急程度越高。调用 FreeRTOS `...FromISR()`
API 的 ISR 必须满足该端口的 syscall 优先级规则；有意设置在 syscall 阈值以上的中断
不得调用 FreeRTOS API。

不要复制案例里的固定优先级。必须根据当前 `FreeRTOSConfig`、NVIC 优先级分组/实现位、
HAL timebase 配置和 ISR 实现计算关系。

## 解释纪律

- 在 tick 等待附近观察到非零 BASEPRI，只能支持“中断被屏蔽”假设，不能单独证明。
- 确认 tick 是否真的停止，以及相关 IRQ 是否 pending 或被屏蔽。
- 同时检查其他原因：定时器时钟关闭、SysTick/定时器停止、全局关中断、调度器状态、
  tick 数据损坏或硬件阻塞。
- 优先采用不破坏 RTOS 中断契约的修复，然后重新构建并复现原场景。
