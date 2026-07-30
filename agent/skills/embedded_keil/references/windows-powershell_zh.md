# Windows 与 PowerShell 执行

英文版本：[windows-powershell.md](windows-powershell.md)。

在 Windows 上优先使用项目脚本和 PowerShell 原生调用方式。

## 同步运行批处理脚本

```powershell
& 'C:\path with spaces\build_keil.bat'
$keilExitCode = $LASTEXITCODE
```

必须立即保存 `$LASTEXITCODE`，不要先调用其他原生进程。

确实需要 `Start-Process` 时，同时使用 `-Wait` 和 `-PassThru`，然后读取返回进程对象的
`ExitCode`。不能启动后台构建后立刻读取可能陈旧的日志。

一个批处理文件调用另一个批处理文件时使用 `call`，否则控制流可能无法按预期返回。

## 路径

- 使用绝对路径，或根据脚本自身目录解析路径。
- 所有可能包含空格或非 ASCII 字符的路径都加引号。
- PowerShell 文件操作使用 `-LiteralPath`。
- 不要假设当前工作目录就是仓库根目录。
- 如果脚本的根目录计算依赖原文件夹层级，迁移脚本时必须同步更新。

## 编码

Windows PowerShell 显示乱码时，显式按 UTF-8 读取文档：

```powershell
Get-Content -LiteralPath 'C:\path\document.md' -Encoding UTF8
```

控制台乱码不能证明文件损坏。重写文件前先诊断解码方式。

## 日志和并发

- 使用明确的日志路径。
- 确认没有其他活动任务正在写同一个 `build_log.txt` 或 `flash_log.txt`。
- 进程完成后再读取日志。
- 固定文件名日志只能视为最近一次运行状态，不能视为历史记录。

## 结果报告

报告准确的脚本或命令、退出码、关键日志摘要、产物状态，以及影响结果解释的环境警告。
