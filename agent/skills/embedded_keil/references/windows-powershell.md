# Windows and PowerShell Execution

Chinese version: [windows-powershell_zh.md](windows-powershell_zh.md).

Prefer project-provided scripts and PowerShell-native invocation on Windows.

## Run a batch script synchronously

```powershell
& 'C:\path with spaces\build_keil.bat'
$keilExitCode = $LASTEXITCODE
```

Capture `$LASTEXITCODE` immediately, before invoking another native process.

When `Start-Process` is necessary, use both `-Wait` and `-PassThru`, then inspect
the returned process object's `ExitCode`. Do not launch a background build and
immediately read a stale log.

Inside one batch file, invoke another batch file with `call`; otherwise control
may not return to the caller as intended.

## Paths

- Use absolute paths or resolve paths from the script's own directory.
- Quote every path that may contain spaces or non-ASCII characters.
- Use `-LiteralPath` for PowerShell file operations.
- Do not assume that the current working directory is the repository root.
- Do not transplant a script whose root calculation depends on its old folder
  layout without updating that calculation.

## Encoding

Read UTF-8 documentation explicitly when Windows PowerShell displays mojibake:

```powershell
Get-Content -LiteralPath 'C:\path\document.md' -Encoding UTF8
```

Console mojibake does not prove that the file is corrupt. Diagnose decoding
before rewriting a file.

## Logs and concurrency

- Use an explicit log path.
- Ensure another active task is not writing the same `build_log.txt` or
  `flash_log.txt`.
- Read the log only after the process has completed.
- Treat fixed-name logs as latest-run state, not historical records.

## Reporting

Report the exact script or command, exit code, important log summary, artifact
state, and any environment warning that affected interpretation.
