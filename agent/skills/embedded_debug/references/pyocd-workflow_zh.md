# pyOCD 工作流

英文版本：[pyocd-workflow.md](pyocd-workflow.md)。

## 发现工具与目标

优先使用项目文档指定的入口。Windows 上 `pyocd.exe` 不在 PATH、但 Python 模块已安装时，
使用：

```powershell
python -m pyocd --version
python -m pyocd list
python -m pyocd list --targets
```

探针枚举退出码为零还不够；输出中必须存在与目标 UID 匹配的探针。存在多个探针时不得
自动选择。

不要假设通用 `cortex_m` target 支持器件 Flash、完整内存映射或外设描述。应根据
请求的操作和已安装 pyOCD 版本选择经过验证的 target。

## 参数化连接

根据发现结果构造命令：

```text
--target <target> --uid <probe-uid> --frequency <swd-frequency>
```

SWD/JTAG 频率必须适合当前接线和目标。降低频率有时可缓解信号完整性或连接问题，但不能
替代正确的 target 和探针选择。

## 复位

复位会改变目标执行状态并破坏易失性故障现场。只有用户请求包含复位，或已接受影响时
才能执行。

```powershell
python -m pyocd reset `
  --target <target> `
  --uid <probe-uid> `
  --frequency <swd-frequency> `
  --method sw `
  --no-wait
```

除非项目证据支持其他方法，优先 `sw` 或 `sysresetreq`。只有 NRST 物理接线确认后才能
使用 `hw`。`--no-wait` 控制没有探针时是否等待连接，并不表示 MCU 复位异步完成。

## 短时寄存器快照

使用一次性 commander 命令，并以 `go` 结束：

```powershell
python -m pyocd commander `
  --target <target> `
  --uid <probe-uid> `
  --frequency <swd-frequency> `
  --elf '<firmware.elf-or-axf>' `
  -c halt `
  -c 'reg pc' `
  -c 'reg lr' `
  -c 'reg sp' `
  -c 'reg xpsr' `
  -c 'reg basepri' `
  -c 'reg primask' `
  -c go
```

commander 的恢复运行命令是 `go`。如果命令在 `go` 前失败，运行单独验证过的恢复命令；
恢复得到确认前，应报告目标可能仍处于暂停状态。

## 地址验证后再读取内存

根据匹配的 ELF/AXF、map、SVD 或器件手册确定变量/寄存器地址，然后使用正确宽度：

```powershell
python -m pyocd commander <connection-arguments> `
  -c halt `
  -c 'read32 <confirmed-address>' `
  -c go
```

先确认外设寄存器是否存在读清除或其他读取副作用。没有明确目的和授权时，不得写内存
或寄存器。

## 验证结果

- 记录准确命令和退出码。
- 在输出中确认所选探针和 target。
- 要求所有请求的寄存器都有值。
- 快照后确认 `go` 成功。
- 复位后分别验证启动标志和持续心跳。
- pyOCD 连接/复位成功不等于固件健康。

如果 Keil Debug 或其他会话占用同一探针，先退出占用者。
