# 嵌入式硬件调试

英文版本及 Codex 正式入口：[SKILL.md](SKILL.md)。

## 语言选择

- 默认读取英文 `.md` 文件。
- 只有在需要确认中文措辞、用户意图、翻译或英文歧义时，才读取对应 `_zh.md`。
- 默认不同时加载两种语言版本，因为它们包含等价规则。
- 修改稳定规则时同步更新中英文版本。

目标仓库、当前固件产物、已连接硬件和真实工具输出是事实来源。不得把其他项目的
芯片、探针 UID、SWD 频率、target 名、符号路径、中断优先级或复位接线当作默认值。

## 安全门槛

1. 执行前确认操作会改变什么状态。
2. 从离线证据和低干扰证据开始。
3. 用户正在保留故障现场或进行时序敏感实验时，除非已理解并接受影响，否则不要
   暂停或复位目标。
4. 每次临时 `halt` 都必须有尽快执行 `go` 的恢复路径。如果采集在恢复前失败，单独
   尝试恢复并报告不确定性。
5. 写内存、写寄存器、修改选项字节、擦除 Flash 或烧录固件前，必须有明确目的和授权。
6. 只有确认探针 NRST 已连接目标复位脚后，才能使用硬件复位。

执行任何暂停、复位或目标写入操作前，读取
[references/safety-boundaries_zh.md](references/safety-boundaries_zh.md)。

## 工作流

1. 优先读取最近的项目 Agent 索引和项目本地调试文档。
2. 发现固件身份、ELF/AXF/map、芯片、探针、唯一 ID、传输方式、工具/版本、复位
   接线和可能占用探针的其他调试会话。
3. 先记录症状时间线并收集低干扰证据：构建身份、串口最后日志、断言、心跳、探针
   枚举结果和问题可重复性。
4. 将症状分类为卡死/等待、Fault、复位循环、探针冲突、外设状态或电气/时序问题。
5. 选择能够区分主要假设的最小诊断动作。
6. 采集证据；适用时恢复执行，并验证系统是否恢复。
7. 分开报告已观察事实、基于证据的推断和尚未验证的假设。

环境发现和相邻工作流路由见
[references/discovery-and-routing_zh.md](references/discovery-and-routing_zh.md)；诊断决策树见
[references/evidence-and-triage_zh.md](references/evidence-and-triage_zh.md)。

## 工具路由

- pyOCD 与 CMSIS-DAP/DAPLink：读取
  [references/pyocd-workflow_zh.md](references/pyocd-workflow_zh.md)。
- Cortex-M Fault、中断屏蔽、FreeRTOS 或 HAL tick 卡死：读取
  [references/cortex-m-faults-and-rtos_zh.md](references/cortex-m-faults-and-rtos_zh.md)。
- 总线时序、脉宽、复位时序或电气行为：读取
  [references/signal-capture_zh.md](references/signal-capture_zh.md)。
- Keil 编译或烧录：使用项目 Keil 工作流或 Keil Skill，不在这里重复。
- 串口协议控制、遥测或波形绘图：使用项目串口工作流或串口 Skill。
- 不得编造尚未在当前环境验证的 ST-Link、J-Link、OpenOCD 或厂商工具命令。

## 结果报告

包括：

- 使用的固件/工程身份和符号文件。
- 实际观察到的芯片、探针、UID、传输方式、工具和版本。
- 执行的准确命令或检查，以及退出状态。
- 目标是否被连接、暂停、恢复或复位。
- 采集的寄存器、Fault 状态、日志或波形证据。
- 事实、推断和剩余假设。
- 下一项最小区分性测试。
- 风险、缺失配置、探针冲突或恢复不确定性。

不能根据单次 PC 快照就声称找到根因，也不能因为复位或探针连接成功就声称固件健康。
软件证据无法解释电气行为时，应请求有针对性的逻辑分析仪或示波器采集，而不是继续
根据代码猜测。
