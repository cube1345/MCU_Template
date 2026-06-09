param(
    [ValidateSet("probe", "flash", "gdbserver")]
    [string]$Command = "probe",

    [ValidateSet("auto", "daplink", "stlink")]
    [string]$Probe = "auto",

    [switch]$Build,

    [string]$ElfPath = "build/Debug/Template.elf",

    [int]$AdapterSpeed = 4000
)

$ErrorActionPreference = "Stop"

function Get-ToolPath {
    param(
        [string]$EnvName,
        [string]$Fallback
    )

    $value = [Environment]::GetEnvironmentVariable($EnvName)
    if (-not [string]::IsNullOrWhiteSpace($value)) {
        return $value
    }

    return $Fallback
}

function Get-PnpText {
    if (Get-Command Get-PnpDevice -ErrorAction SilentlyContinue) {
        return (Get-PnpDevice -PresentOnly | Out-String)
    }

    return (Get-CimInstance Win32_PnPEntity | Out-String)
}

function Resolve-Probe {
    param([string]$RequestedProbe)

    if ($RequestedProbe -ne "auto") {
        return $RequestedProbe
    }

    $pnpText = Get-PnpText
    $hasDapLink = ($pnpText -match "DAPLink") -or ($pnpText -match "CMSIS-DAP")
    $hasStLink = ($pnpText -match "ST-Link") -or ($pnpText -match "STLINK")

    if ($hasDapLink -and $hasStLink) {
        throw "Both DAPLink/CMSIS-DAP and ST-Link were detected. Re-run with -Probe daplink or -Probe stlink."
    }

    if ($hasDapLink) {
        return "daplink"
    }

    if ($hasStLink) {
        return "stlink"
    }

    throw "No DAPLink/CMSIS-DAP or ST-Link probe was detected."
}

function Invoke-OpenOcd {
    param(
        [string]$OpenOcdExe,
        [string]$OpenOcdScripts,
        [string[]]$OpenOcdArgs
    )

    Write-Host ("openocd " + ($OpenOcdArgs -join " "))
    & $OpenOcdExe @OpenOcdArgs
    return $LASTEXITCODE
}

if ($Build) {
    cmake --preset Debug
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    cmake --build build\Debug
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$selectedProbe = Resolve-Probe -RequestedProbe $Probe
$openocdExe = Get-ToolPath -EnvName "OPENOCD_EXE" -Fallback "openocd"
$openocdScripts = Get-ToolPath -EnvName "OPENOCD_SCRIPTS" -Fallback ""

$commonArgs = @()
if (-not [string]::IsNullOrWhiteSpace($openocdScripts)) {
    $commonArgs += @("-s", $openocdScripts)
}

if ($selectedProbe -eq "daplink") {
    $commonArgs += @("-f", "interface/cmsis-dap.cfg", "-f", "target/stm32f4x.cfg", "-c", "transport select swd")
} else {
    $commonArgs += @("-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg", "-c", "transport select hla_swd")
}

$commonArgs += @("-c", "adapter speed $AdapterSpeed")

switch ($Command) {
    "probe" {
        $args = $commonArgs + @("-c", "init", "-c", "targets", "-c", "reset halt", "-c", "exit")
        $exitCode = Invoke-OpenOcd -OpenOcdExe $openocdExe -OpenOcdScripts $openocdScripts -OpenOcdArgs $args
        if (($exitCode -ne 0) -and ($selectedProbe -eq "stlink")) {
            $fallbackArgs = $args -replace "transport select hla_swd", "transport select swd"
            $exitCode = Invoke-OpenOcd -OpenOcdExe $openocdExe -OpenOcdScripts $openocdScripts -OpenOcdArgs $fallbackArgs
        }
        exit $exitCode
    }

    "flash" {
        if (-not (Test-Path $ElfPath)) {
            throw "ELF file not found: $ElfPath. Re-run with -Build or build the project first."
        }

        $args = $commonArgs + @("-c", "program $ElfPath verify reset exit")
        $exitCode = Invoke-OpenOcd -OpenOcdExe $openocdExe -OpenOcdScripts $openocdScripts -OpenOcdArgs $args
        if (($exitCode -ne 0) -and ($selectedProbe -eq "stlink")) {
            $fallbackArgs = $args -replace "transport select hla_swd", "transport select swd"
            $exitCode = Invoke-OpenOcd -OpenOcdExe $openocdExe -OpenOcdScripts $openocdScripts -OpenOcdArgs $fallbackArgs
        }
        exit $exitCode
    }

    "gdbserver" {
        $args = $commonArgs
        Invoke-OpenOcd -OpenOcdExe $openocdExe -OpenOcdScripts $openocdScripts -OpenOcdArgs $args
        exit $LASTEXITCODE
    }
}
