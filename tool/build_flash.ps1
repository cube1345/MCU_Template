<#
 .SYNOPSIS
 Rebuild and flash the STM32F407 Keil project.

 .DESCRIPTION
 This script follows the project batch workflow: call UV4 directly with its
 batch build or flash command, then read the corresponding Keil log.

 .AUTHOR
 weixin

 .EXAMPLE
 .\tool\build_flash.ps1

 .EXAMPLE
 .\tool\build_flash.ps1 -BuildOnly
#>
[CmdletBinding()]
param(
  [string] $KeilPath = 'D:\Keil_v5-537\UV4\UV4.exe',
  [switch] $BuildOnly
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repoRoot 'MDK-ARM\empty.uvprojx'
$projectDir = Split-Path -Parent $projectPath
$outputDir = Join-Path $projectDir 'empty'
$buildLog = Join-Path $outputDir 'codex_build.log'
$flashLog = Join-Path $outputDir 'codex_flash.log'
$targetName = 'empty'

function Invoke-Keil {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Command,
    [Parameter(Mandatory = $true)]
    [string] $LogPath,
    [Parameter(Mandatory = $true)]
    [string] $CompletedText
  )

  if (Test-Path -LiteralPath $LogPath) {
    Remove-Item -LiteralPath $LogPath -Force
  }

  # Keep the direct UV4 batch command used by agent/keil/*.bat. A hidden cmd
  # parent supplies the startup state to UV4 and prevents a command window
  # from being shown while still waiting for the build or flash to finish.
  $cmdCommand = 'call "{0}" {1} "{2}" -t "{3}" -j0 -o "{4}"' -f `
                $KeilPath, $Command, $projectPath, $targetName, $LogPath
  $cmdProcess = Start-Process -FilePath $env:ComSpec `
                              -ArgumentList @('/d', '/c', $cmdCommand) `
                              -WindowStyle Hidden -Wait -PassThru
  $keilExitCode = $cmdProcess.ExitCode
  # UV4 returns 1 for a successful batch build on this ARMCC project. The
  # reference Keil script treats codes 0 and 1 as build completion.
  if (($Command -eq '-b') -and ($keilExitCode -ge 2)) {
    throw "Keil command failed with exit code $keilExitCode."
  }
  if (($Command -eq '-f') -and ($keilExitCode -ne 0)) {
    throw "Keil command failed with exit code $keilExitCode."
  }
  if (-not (Test-Path -LiteralPath $LogPath)) {
    throw "Keil did not create the log: $LogPath"
  }
  $content = Get-Content -Raw -LiteralPath $LogPath
  if ($content -notmatch [Regex]::Escape($CompletedText)) {
    throw "Keil log does not contain the completion marker: $CompletedText"
  }
  Write-Host $content
  return $content
}

if (-not (Test-Path -LiteralPath $KeilPath)) {
  throw "Keil was not found: $KeilPath"
}
if (-not (Test-Path -LiteralPath $projectPath)) {
  throw "Keil project was not found: $projectPath"
}

# -b is Keil's batch-build command: it builds and exits instead of entering
# the interactive IDE workflow used by -r.
$buildResult = Invoke-Keil -Command '-b' -LogPath $buildLog `
                           -CompletedText 'Build Time Elapsed:'
if ($buildResult -notmatch '0 Error\(s\), 0 Warning\(s\)') {
  throw 'Build failed. Check the Keil log above.'
}

$hexPath = Join-Path $outputDir 'empty.hex'
if (-not (Test-Path -LiteralPath $hexPath)) {
  throw "Build succeeded but HEX was not found: $hexPath"
}

if ($BuildOnly) {
  Write-Host "Build succeeded: $hexPath"
  exit 0
}

$flashResult = Invoke-Keil -Command '-f' -LogPath $flashLog `
                           -CompletedText 'Flash Load finished'
if ($flashResult -notmatch 'Verify OK') {
  throw 'Flash or verification failed. Check the CMSIS-DAP/ST-Link connection.'
}

Write-Host "Flash succeeded: $hexPath"
