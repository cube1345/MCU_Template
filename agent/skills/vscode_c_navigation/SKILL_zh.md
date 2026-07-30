---
name: vscode-c-navigation
description: 诊断、修复并验证 Visual Studio Code 中 C/C++ 工程的声明、定义、引用和头文件跳转，尤其适用于 Keil、STM32、CMSIS、HAL、FreeRTOS 及其他嵌入式工程。当 F12、Ctrl+单击、转到声明、转到定义、查找所有引用、语义补全或头文件跳转失效；IntelliSense 未启动或使用了错误的宏、头文件路径、编译器；或者纯 Keil 工程需要在不改变真实固件构建流程的前提下用 CMake 安全生成 compile_commands.json 时使用。
---

# VSCode C 语言跳转

英文版本：[SKILL.md](SKILL.md)。

## 语言选择

- 默认读取英文文件。
- 仅在需要中文措辞、翻译核对或消除歧义时读取 `SKILL_zh.md`。
- 修改本技能时，必须保持两个文件的语义同步。

以目标仓库、仓库内的 agent 规范和真实构建工程为准。参考工程只能作为证据，
不得不加判断地照搬。

## 按顺序诊断

不要一开始就反复修改 `includePath`。首先确认语言服务是否真正运行。

1. 确认准确的 VSCode 窗口和工作区根目录。使用 `code --status`、进程命令行
   或可见窗口标题。不要在第二个欢迎窗口或仓库的另一个副本上诊断。
2. 确认当前文件的语言模式是 `C` 或 `C++`。
3. 确认 `ms-vscode.cpptools` 已安装，且**在当前工作区启用**。扩展即使已经安装，
   详情页仍可能显示 `启用(工作区)`。必须先启用，再修改工程配置。
4. 检查工作区信任。如果 VSCode 询问是否信任文件夹，应按照当前 UI 自动化策略
   获取所需确认后再选择 `信任文件夹并继续`，不得绕过信任提示。
5. 检查用户设置和工作区设置中是否存在：
   - `C_Cpp.intelliSenseEngine: "disabled"`；
   - 失效的 `configurationProvider`；
   - 缺失或过期的 `compileCommands` 路径；
   - 与 `clangd`、EIDE、PlatformIO 或其他语言服务器的冲突。
6. 检查当前扩展宿主日志和 C/C++ 输出日志。确认 `ms-vscode.cpptools` 已激活，
   并且 `cpptools.exe` 或其服务进程已启动。没有激活记录比 JSON 配置正确更能说明问题。

在命令可用时运行 `C/C++: Log Diagnostics`。如果命令面板中不存在该命令，
应将其视为扩展被禁用、工作区未信任、扩展未在该窗口注册或尚未激活的证据。

## 选择配置策略

对于非简单工程，优先使用真实的编译数据库。

### 已有编译数据库

如果真实构建系统已经导出 `compile_commands.json`，让 C/C++ 直接读取它，
不要再建立一套平行的构建描述。

### 纯 Keil 或其他厂商 IDE 的嵌入式工程

当工程没有编译数据库时，创建仅用于索引的 `CMakeLists.txt`：

- 保持 Keil 或厂商 IDE 继续负责真实构建、链接、下载和调试。
- CMake 只用于生成 `compile_commands.json`。
- 根据真实工程同步已启用源码、头文件目录、预处理宏、语言标准、MCU、FPU 和 ABI。
- 优先列出真实启用的源码，不要把厂商驱动目录内所有模块全部通配进来。
- 使用 `CMAKE_SYSTEM_NAME Generic`、
  `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` 和
  `CMAKE_EXPORT_COMPILE_COMMANDS ON`。
- 使用系统现有的 ARM GCC/Clang 工具链做索引。必须探测真实路径，不能照搬其他机器。
- Windows 上传给 CMake 的路径使用正斜杠。如果失败缓存含有反斜杠转义，
  使用 `--fresh` 重新配置。

ARMCC 专用 FreeRTOS 移植层或内联汇编文件可能导致可选的 GCC 索引编译失败，
但编译数据库仍然正确。不得为了让仅索引目标通过编译而修改生产固件。
导航的判定条件是 CMake 配置成功且数据库内容正确。

### C/C++ 直接配置

当 provider 激活顺序不可靠时，优先让 C/C++ 直接读取数据库：

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/cmake-build-vscode/compile_commands.json",
  "C_Cpp.intelliSenseEngine": "default"
}
```

在以下情况下添加 `.vscode/c_cpp_properties.json`：

- 触发 C/C++ 工作区激活；
- 直接声明 `compileCommands`、`compilerPath` 和 `intelliSenseMode`；
- 提供备用的 `defines`、`includePath` 和 `browse.path`。

不得保留互相矛盾的编译器路径、编译数据库或 configuration provider。
直接配置 `compileCommands` 后，不应继续等待未激活的 CMake provider。

## 安全实施

1. 编辑前阅读仓库内的 agent 文件。
2. 保留用户已有改动和真实 IDE 工程。
3. 只添加通常必需的最少索引文件：
   - 仅索引工程使用的 `CMakeLists.txt`；
   - `.vscode/settings.json`；
   - 需要直接激活或备用配置时使用的 `.vscode/c_cpp_properties.json`；
   - 用于重新生成数据库的 `.vscode/tasks.json`；
   - 用于推荐扩展的 `.vscode/extensions.json`；
   - `.gitignore` 中的生成目录条目。
4. 立即生成编译数据库，不能只交付一个尚未配置过的 CMake 文件。
5. 不要猜测使用 `code --enable-extension` 等不受支持的 CLI 参数；
   应通过受支持的 VSCode UI 或命令启用、禁用扩展。

## 用证据验证

完成所有适用检查：

1. 解析全部已编辑的 JSON 文件。
2. 确认 CMake 配置退出码为零。
3. 解析 `compile_commands.json`，验证应用、platform、driver、RTOS 和厂商 HAL
   各层的代表文件均存在记录。
4. 抽查一条编译命令，确认编译器、MCU 参数、宏和头文件目录正确。
5. 重新加载目标 VSCode 窗口。
6. 从日志确认 C/C++ 扩展已经激活，并确认语言服务进程启动。
7. 运行 `C/C++: Log Diagnostics`，确认选中了预期配置和编译数据库。
8. 对已知工程符号执行 F12 或 Ctrl+单击，双向验证：声明跳定义，以及调用点跳声明或定义。
9. 工程适用时，再测试一个 HAL 或 RTOS 框架符号。

不能因为 `compile_commands.json` 存在就报告成功。只有当前 VSCode 窗口消费了配置，
并且真实符号跳转成功，才算修复完成。

## 报告

报告以下内容：

- 实际根因；
- 修改的文件或 VSCode 状态；
- 是否需要启用工作区扩展或信任工作区；
- 编译数据库条目数和代表性覆盖范围；
- 最终用于跳转验证的具体符号；
- 不影响真实构建的仅索引编译器兼容性问题。

