@echo off
setlocal EnableExtensions

if /I not "%SYSCONFIG_REGENERATE%"=="1" (
    echo [SysConfig] Using checked-in ti_msp_dl_config.*. Set SYSCONFIG_REGENERATE=1 to regenerate.
    exit /b 0
)

if defined SYSCONFIG_CLI (
    set "SYS_CFG_CLI=%SYSCONFIG_CLI%"
) else (
    set "SYS_CFG_CLI=E:\EmbeddedDevelopment\SoftWare\Tools\sysconfig_1.25.0\sysconfig_cli.bat"
)

if not exist "%SYS_CFG_CLI%" (
    echo [ERROR] SysConfig CLI was not found. Set SYSCONFIG_CLI to sysconfig_cli.bat.
    exit /b 1
)

set "SYS_CFG_FILE=%~dp0empty.syscfg"
if not "%~2"=="" if exist "%~dp0%~2" set "SYS_CFG_FILE=%~dp0%~2"

echo [SysConfig] Regenerating from %SYS_CFG_FILE%
call "%SYS_CFG_CLI%" --script "%SYS_CFG_FILE%" --output "%~dp0"
exit /b %ERRORLEVEL%
