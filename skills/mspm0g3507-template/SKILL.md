---
name: mspm0g3507-template
description: Build, validate, or refactor this MSPM0G3507 Keil firmware template. Use when changing layered drivers, SysConfig, the Keil project, CMake source manifest, or repository-local firmware documentation.
---

# MSPM0G3507 Template

Use this workflow for this repository only. Keep the five-layer layout; prefer
injected hardware ports and static ops tables over direct DriverLib calls in a
Device or App module.

## Read first

1. Read `docs/myused_five_layer_architecture.md` and `docs/project_status.md`.
2. Run `git status --short --branch`; never stage or discard existing user changes.
3. When an API, pin, clock, or DriverLib type matters, verify it in
   `ti_msp_dl_config.*`, `empty.syscfg`, and the local TI SDK before editing.

## Layer contract

- `0_hal`: the only reusable DriverLib adapter layer.
- `1_core`: status types and narrow cross-device contracts.
- `2_device`: reusable state and algorithms; no Board includes, pin macros, or `DL_*` calls.
- `3_board`: board instances, SysConfig macros, IRQ entry, and port adapters.
- `4_facade`: stable application and legacy entry points only.
- `5_libs` and `6_apps`: algorithms and product behavior; no register access.

Use function-pointer ops only where hardware or behavior can vary. Keep ops
tables `static const`, keep callback context explicit, and do not allocate from
an ISR or a real-time control path.

## SysConfig and build

- `empty.syscfg` is the source configuration; `ti_msp_dl_config.*` is generated output.
- The checked-in output is used by default. Set `SYSCONFIG_REGENERATE=1` only
  when deliberately regenerating it through `syscfg.bat`.
- The supported CLI is selected from `SYSCONFIG_CLI`; the local fallback is
  `E:\EmbeddedDevelopment\SoftWare\Tools\sysconfig_1.25.0\sysconfig_cli.bat`.
- The official TI SysConfig landing page is https://www.ti.com/tool/SYSCONFIG.
- Use `E:\EmbeddedDevelopment\SoftWare\Keil5\Keil5_Core\UV4\UV4.exe -r keil\MSPG3507_FreeRTOS.uvprojx -j1 -o build_log.txt` for a clean software build.
- Verify the final log contains `- 0 Error(s)`; process exit code alone is not sufficient.
- Do not flash by default. `run_keil.bat flash` is the explicit hardware action.

## Change and verification rules

1. Update `CMakeLists.txt` and `keil/MSPG3507_FreeRTOS.uvprojx` together when a compiled source file is added or removed.
2. Add Doxygen for public APIs, callbacks, ISR entry points, and hardware assumptions.
3. Run `git diff --check`, a clean Keil rebuild, and a CMake configure check after structural changes.
4. Record decisions in `docs/decisions/` and update `docs/project_status.md` when priorities or constraints change.
5. Make small, staged-only commits. Use a Japanese `add:`, `fix:`, `clean:`, `docs:`, or `update:` prefix; do not push automatically.

## Hardware boundary

Build success does not validate pin mux, PWM polarity, I2C ACK, interrupt timing,
or board clocking. Record those as hardware smoke tests unless verified on the
target board.
