@echo off
setlocal EnableExtensions

set "BASE_DIR=%~dp0"
set "KEIL_PATH=E:\EmbeddedDevelopment\SoftWare\Keil5\Keil5_Core\UV4\UV4.exe"
set "PROJECT_PATH=%BASE_DIR%keil\MSPG3507_FreeRTOS.uvprojx"
set "BUILD_LOG=%BASE_DIR%build_log.txt"

if not exist "%KEIL_PATH%" (
    echo [ERROR] UV4.exe was not found: %KEIL_PATH%
    exit /b 1
)

if not exist "%PROJECT_PATH%" (
    echo [ERROR] Keil project was not found: %PROJECT_PATH%
    exit /b 1
)

echo [1/2] Clean building MSPM0G3507 firmware...
"%KEIL_PATH%" -r "%PROJECT_PATH%" -j1 -o "%BUILD_LOG%"
set "UV4_EXIT=%ERRORLEVEL%"

if not exist "%BUILD_LOG%" (
    echo [ERROR] Build log was not created.
    exit /b 1
)

findstr /C:"- 0 Error(s)" "%BUILD_LOG%" >nul
if errorlevel 1 (
    echo [ERROR] Keil reported compilation errors. See %BUILD_LOG%
    exit /b 1
)

if not "%UV4_EXIT%"=="0" (
    echo [WARN] UV4 returned %UV4_EXIT%, but the build log reports zero errors.
)

if /I "%~1"=="flash" (
    echo [2/2] Flashing explicitly requested...
    "%KEIL_PATH%" -f "%PROJECT_PATH%" -j1 -o "%BASE_DIR%flash_log.txt"
    if errorlevel 1 (
        echo [ERROR] Keil flash command failed. See %BASE_DIR%flash_log.txt
        exit /b 1
    )
) else (
    echo [2/2] Flash skipped. Use "run_keil.bat flash" only with connected hardware.
)

exit /b 0
