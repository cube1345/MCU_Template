# 命令行编译、烧录与调试说明

本文档记录当前工程在 Windows PowerShell 下使用 CMake、STM32CubeCLT、OpenOCD、DAPLink/CMSIS-DAP 进行编译、烧录和调试的命令。

## 1. 工具位置

当前项目使用的主要工具如下：

```powershell
# 项目根目录
E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template

# CMake
E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\CMake\bin\cmake.exe

# Ninja
C:\Users\Cube\AppData\Local\stm32cube\bundles\ninja\1.13.2+st.1\bin\ninja.exe

# GCC / GDB 工具链
E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe
E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gdb.exe
E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-objcopy.exe
E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-size.exe

# OpenOCD
E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe

# OpenOCD scripts
E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts
```

本项目当前使用的 OpenOCD 配置：

```text
interface/cmsis-dap.cfg
target/stm32f4x.cfg
```

如果使用 ST-Link，把接口配置换成：

```text
interface/stlink.cfg
target/stm32f4x.cfg
```

## 2. 编译

进入项目根目录：

```powershell
cd E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template
```

配置 Debug 构建目录：

```powershell
cmake --preset Debug
```

编译 Debug 固件：

```powershell
cmake --build build\Debug
```

查看详细编译命令：

```powershell
cmake --build build\Debug --verbose
```

编译产物：

```text
build\Debug\Template.elf
```

当前工程已经在 `CMakeLists.txt` 中把以下目录纳入构建：

```text
0_hal
1_core
2_device
3_board
4_facade
5_libs
6_apps
Core
Drivers
```

因此 `cmake --build build\Debug` 会覆盖当前五层架构代码，而不是只编译 CubeMX 生成文件。

## 3. 烧录

先确认 DAPLink/CMSIS-DAP 已连接：

```powershell
pyocd list
```

本机当前识别到的 DAPLink 示例：

```text
Horco Horco CMSIS-DAP
```

当前模板推荐使用 OpenOCD + `target/stm32f4x.cfg` 烧录 STM32F401。

### 3.1 OpenOCD 探测目标

```powershell
& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface\cmsis-dap.cfg `
  -f target\stm32f4x.cfg `
  -c 'transport select swd' `
  -c 'adapter speed 4000' `
  -c 'init' `
  -c 'targets' `
  -c 'reset halt' `
  -c 'exit'
```

成功时应能看到类似信息：

```text
Cortex-M4 processor detected
Examination succeed
halted due to debug-request
```

### 3.2 烧录并校验

```powershell
& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface\cmsis-dap.cfg `
  -f target\stm32f4x.cfg `
  -c 'transport select swd' `
  -c 'adapter speed 4000' `
  -c 'program build/Debug/Template.elf verify reset exit'
```

成功烧录时应看到：

```text
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

### 3.3 使用 ST-Link 烧录

ST-Link 和 DAPLink 的主要区别是 OpenOCD interface 配置不同：

```powershell
& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface/stlink.cfg `
  -f target/stm32f4x.cfg `
  -c 'transport select hla_swd' `
  -c 'adapter speed 4000' `
  -c 'program build/Debug/Template.elf verify reset exit'
```

如果当前 OpenOCD 版本提示 `hla_swd` 不可用，可改成：

```powershell
-c 'transport select swd'
```

如果连接不稳定，先把 SWD 速度降到：

```powershell
-c 'adapter speed 1000'
```

ST-Link SWD 基本接线：

```text
ST-Link SWDIO -> STM32 PA13 / SWDIO
ST-Link SWCLK -> STM32 PA14 / SWCLK
ST-Link GND   -> 目标板 GND
ST-Link 3.3V  -> 目标板 3.3V 参考电压
```

如果需要 connect under reset，需要额外连接 `NRST`，并在 OpenOCD 命令中增加：

```powershell
-c 'reset_config srst_only srst_nogate connect_assert_srst'
```

### 3.4 自动选择 DAPLink 或 ST-Link

项目提供了一个 PowerShell 脚本：

```text
scripts/openocd_probe_auto.ps1
```

它会在 Windows 当前已连接设备中查找：

```text
DAPLink / CMSIS-DAP
ST-Link / STLINK
```

只检测到 DAPLink/CMSIS-DAP 时，使用：

```text
interface/cmsis-dap.cfg
transport select swd
```

只检测到 ST-Link 时，使用：

```text
interface/stlink.cfg
transport select hla_swd
```

如果 ST-Link 使用 `hla_swd` 失败，脚本会自动再尝试 `transport select swd`。

首次使用前，如果 `openocd.exe` 不在系统 `PATH`，可以在当前 PowerShell 窗口设置：

```powershell
$env:OPENOCD_EXE = 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe'
$env:OPENOCD_SCRIPTS = 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts'
```

自动探测并烧录：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command flash
```

自动编译并烧录：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command flash -Build
```

只探测并复位停住目标：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command probe
```

启动 GDB Server：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command gdbserver
```

如果电脑同时插着 DAPLink 和 ST-Link，脚本会停止并要求显式选择，避免烧录到错误目标：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command flash -Probe daplink
.\scripts\openocd_probe_auto.ps1 -Command flash -Probe stlink
```

## 4. 一键编译并烧录

可直接使用下面命令：

```powershell
cd E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template

cmake --preset Debug
cmake --build build\Debug

& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface\cmsis-dap.cfg `
  -f target\stm32f4x.cfg `
  -c 'transport select swd' `
  -c 'adapter speed 4000' `
  -c 'program build/Debug/Template.elf verify reset exit'
```

也可以使用自动探测脚本：

```powershell
cd E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template

$env:OPENOCD_EXE = 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe'
$env:OPENOCD_SCRIPTS = 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts'

.\scripts\openocd_probe_auto.ps1 -Command flash -Build
```

## 5. 命令行调试

命令行调试分两步：

1. 启动 OpenOCD GDB Server。
2. 用 `arm-none-eabi-gdb` 连接。

### 5.1 启动 OpenOCD GDB Server

在第一个 PowerShell 窗口运行：

```powershell
cd E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template

& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface\cmsis-dap.cfg `
  -f target\stm32f4x.cfg `
  -c 'transport select swd' `
  -c 'adapter speed 4000'
```

默认端口：

```text
GDB:    localhost:3333
Tcl:    localhost:6666
Telnet: localhost:4444
```

如果端口被占用，可指定端口：

```powershell
& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -c 'gdb_port 50000' `
  -c 'tcl_port 50001' `
  -c 'telnet_port 50002' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface\cmsis-dap.cfg `
  -f target\stm32f4x.cfg `
  -c 'transport select swd' `
  -c 'adapter speed 4000'
```

使用 ST-Link 调试时，把 `interface/cmsis-dap.cfg` 换成 `interface/stlink.cfg`：

```powershell
& 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\bin\openocd.exe' `
  -s 'E:\EmbeddedDevelopment\SoftWare\Tools\openocd-20260302\OpenOCD-20260302-0.12.0\share\openocd\scripts' `
  -f interface/stlink.cfg `
  -f target/stm32f4x.cfg `
  -c 'transport select hla_swd' `
  -c 'adapter speed 4000'
```

也可以使用自动探测脚本启动 GDB Server：

```powershell
.\scripts\openocd_probe_auto.ps1 -Command gdbserver
```

然后在另一个 PowerShell 窗口用 GDB 连接 `localhost:3333`。

### 5.2 使用 GDB 连接

在第二个 PowerShell 窗口运行：

```powershell
cd E:\EmbeddedDevelopment\WorkSpace\F401CCU6\Template

& 'E:\EmbeddedDevelopment\SoftWare\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gdb.exe' `
  build\Debug\Template.elf
```

进入 GDB 后执行：

```gdb
target extended-remote localhost:3333
monitor reset halt
load
break main
continue
```

如果 OpenOCD 使用了 `gdb_port 50000`，则连接：

```gdb
target extended-remote localhost:50000
```

常用 GDB 命令：

```gdb
# 复位并停住
monitor reset halt

# 烧录当前 ELF
load

# 在 main 处打断点
break main

# 在 Error_Handler 处打断点
break Error_Handler

# 继续运行
continue

# 单步
step
next

# 查看调用栈
backtrace

# 查看变量
print SystemCoreClock
print uwTick

# 复位并运行
monitor reset run

# 退出
quit
```

## 6. VSCode Cortex-Debug 对应配置

VSCode 的 Cortex-Debug 配置与命令行等价，核心参数如下：

```json
{
  "servertype": "openocd",
  "executable": "${workspaceFolder}/build/Debug/Template.elf",
  "gdbPath": "E:/EmbeddedDevelopment/SoftWare/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin/arm-none-eabi-gdb.exe",
  "serverpath": "E:/EmbeddedDevelopment/SoftWare/Tools/openocd-20260302/OpenOCD-20260302-0.12.0/bin/openocd.exe",
  "searchDir": [
    "E:/EmbeddedDevelopment/SoftWare/Tools/openocd-20260302/OpenOCD-20260302-0.12.0/share/openocd/scripts"
  ],
  "configFiles": [
    "interface/cmsis-dap.cfg",
    "target/stm32f4x.cfg"
  ],
  "interface": "swd",
  "device": "STM32F401CCUx",
  "openOCDLaunchCommands": [
    "transport select swd",
    "adapter speed 4000"
  ],
  "runToEntryPoint": "main"
}
```

使用 ST-Link 时只需要改 OpenOCD interface 和 transport：

```json
{
  "configFiles": [
    "interface/stlink.cfg",
    "target/stm32f4x.cfg"
  ],
  "interface": "swd",
  "openOCDLaunchCommands": [
    "transport select hla_swd",
    "adapter speed 4000"
  ]
}
```

如果 `hla_swd` 不适配当前 OpenOCD 版本，改成：

```json
"openOCDLaunchCommands": [
  "transport select swd",
  "adapter speed 4000"
]
```

注意：

- `"runToEntryPoint": "main"` 会让调试启动后停在 `main`，这是正常行为。
- 如果 `postLaunchCommands` 中设置了 `break Core/Src/main.c:133` 等断点，程序会继续停在对应行，并不是报错。
- 若希望启动后直接运行，可删除 `"runToEntryPoint": "main"`。
- VSCode Cortex-Debug 的 `configFiles` 通常不适合运行时自动切换。建议保留 DAPLink 和 ST-Link 两个 launch 配置；命令行烧录/调试用 `scripts/openocd_probe_auto.ps1` 自动选择。

## 7. 常见问题

### 7.1 pyOCD 能看到 DAPLink，但不能烧 STM32F401

现象：

```text
pyocd list
Target: n/a
```

原因：当前 pyOCD 安装可能未内置 STM32F401 target pack。

处理：使用 OpenOCD + `target/stm32f4x.cfg` 烧录。

### 7.2 OpenOCD 连接失败

检查：

```powershell
pyocd list
```

如果看不到 CMSIS-DAP：

- 检查 DAPLink USB 线是否支持数据传输；
- 检查 SWDIO、SWCLK、GND、3V3、NRST 是否连接正确；
- 检查目标板是否供电；
- 关闭可能占用 DAPLink 的 VSCode 调试会话、OpenOCD、Keil、STM32CubeProgrammer。

### 7.3 OpenOCD 端口被占用

检查端口：

```powershell
Get-NetTCPConnection -LocalPort 3333,4444,6666,50000,50001,50002 -ErrorAction SilentlyContinue |
  Select-Object LocalAddress,LocalPort,State,OwningProcess
```

结束占用进程：

```powershell
Stop-Process -Id <PID> -Force
```

### 7.4 调试时停在 main 或 SystemClock_Config

停在 `main` 通常是因为 VSCode 配置了：

```json
"runToEntryPoint": "main"
```

停在 `SystemClock_Config` 某一行通常是因为手动设置了断点，不是异常。

若真的进入 `Error_Handler`，再检查：

- HSE 晶振是否存在；
- `SystemClock_Config()` 中 HSE/PLL 参数是否符合硬件；
- CubeMX 时钟树是否正确；
- 供电和 BOOT 配置是否正确。
