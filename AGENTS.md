# Repository Guidelines

## Project Structure & Module Organization

This is a C11 MSPM0G3507 firmware template organized around the `HAL -> Core -> Device -> Board -> Facade -> App` flow. `0_hal/` wraps TI DriverLib and MCU peripherals. `1_core/` defines shared device base and ops contracts. `2_device/` contains reusable device logic, state, and configuration structs. `3_board/` binds devices to SysConfig macros, pins, timers, buses, and ISR entry points. `4_facade/` exposes stable application and legacy APIs. `5_libs/` and `6_apps/` hold reusable algorithms and example app behavior. `Mid/` contains FreeRTOS sources, `source/` vendors TI/CMSIS code, `tests/` hosts PC-side unit tests, `keil/` is the authoritative firmware project, and `docs/` records architecture and ADRs.

## Build, Test, and Development Commands

- `run_keil.bat`: clean-builds `keil/MSPG3507_FreeRTOS.uvprojx`; verify `build_log.txt` contains `- 0 Error(s)`.
- `run_keil.bat flash`: flashes hardware explicitly; do not use without a connected target.
- `cmake -S . -B build -G Ninja -DBUILD_TESTING=ON`: configures the bounded CMake source manifest for editor and host checks.
- `cmake --build build --target motor_device_test`: builds the current host test target.
- `ctest --test-dir build --output-on-failure`: runs configured host tests.
- `SYSCONFIG_REGENERATE=1 syscfg.bat`: regenerate `ti_msp_dl_config.*` from `empty.syscfg` only when intended.

## Coding Style & Naming Conventions

Use four-space indentation and existing brace style. Public module APIs use module prefixes such as `Motor_InitDevice` or `HAL_GPIO_WritePin`; types use lower snake case with suffixes like `_t`, `_cfg_t`, and `_ops_t`. Keep private helpers and module state `static`; keep default configs and ops tables `static const`. Device code must avoid Board includes, SysConfig macros, and direct `DL_*` calls; inject hardware through cfg structs, callbacks, or port adapters.

## Testing Guidelines

Place host tests in `tests/*_test.c` and prefer `assert`-based checks around Device behavior with mocked or injected ports. Host tests should not require board hardware. Record hardware smoke coverage separately for pin mux, PWM polarity, I2C ACK, IRQ timing, clocks, motors, and sensors.

## Commit & Pull Request Guidelines

Follow the repository history: concise subjects with prefixes such as `add:`, `fix:`, `clean:`, `docs:`, or `update:`. In PRs, describe touched layers, list build/test commands run, note hardware validation status, and link related issues or ADRs. When adding or removing compiled sources, update both `CMakeLists.txt` and `keil/MSPG3507_FreeRTOS.uvprojx`.

## Security & Configuration Tips

Do not commit generated logs, build outputs, local IDE state, or machine-specific tool paths. Prefer environment variables such as `SYSCONFIG_CLI` for local tool overrides, and keep flashing as an explicit opt-in action.
