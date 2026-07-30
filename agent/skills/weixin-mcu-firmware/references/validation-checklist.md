# Validation Checklist

Use this checklist before handing off firmware changes. Not every item is always executable; report skipped checks clearly.

## 1. Protocol source check

- Read the current MCU UART protocol doc.
- Read the configurable protocol README.
- Read the default or active JSON protocol file.
- Check whether the target firmware project has its own protocol notes.
- Record command prefix, response prefix, line ending, encoding, command templates, plot prefix, and separators.

## 2. Code review check

- UART parsing is not doing expensive work in an ISR.
- Non-protocol lines are ignored or handled according to the active protocol.
- Command buffer overflow is handled deterministically.
- Parameter lookup uses stable protocol names.
- `list` count equals the number of emitted parameter metadata rows.
- `get` and `set` success lines use the exact active response format.
- `set` checks existence, access, parse/type, range, and safety.
- Numeric parsing rejects NaN/Inf, overflow/underflow, fractional integers, trailing characters, and extra command tokens.
- Read-only and dangerous values cannot be changed through ordinary tuning paths.
- Wave output is rate-limited and not printed from timing-critical code.

## 3. Host or compile validation

Run the firmware project's normal validation commands when available:

- formatter/linter, if already configured
- compiler with warnings
- host-side unit tests, if present
- static analysis, if already part of the project

Do not invent a new toolchain setup during a small protocol task unless the user asks.

## 4. Hardware action boundary

Before flash/erase/program/unlock, reset or power-cycle, debugger attach/halt/run, bootloader entry, or option-byte/fuse/protection changes, verify explicit current-task approval for the exact action and record:

- board identity
- firmware image or build target
- programmer/debugger
- port and baud rate
- recovery path if the flash fails
- whether the device is isolated and safe to interrupt/reset

If any item is unknown and the action could affect real hardware, stop and ask.

## 5. Device smoke test

Use harmless checks first:

1. Boot firmware and confirm ordinary startup logs, if any.
2. Send the protocol help command if supported.
3. Run list/scan and verify the reported count.
4. Read a read-only firmware version/status parameter.
5. Read one safe read/write tuning parameter.
6. Write one reversible safe value only when authorized.
7. Restore the original value when appropriate.
8. Observe wave output without increasing telemetry rate beyond the device's safe setting.

## 6. GUI verification through WEIXIN Serial Monitor

Use `agent/skills/weixin-serial-monitor/SKILL.md` for the GUI side. Verify:

- `discover` selects the intended EXE/runtime.
- `status` proves the GUI bridge is live.
- the initial connection state is recorded and restored if this workflow changes it.
- `connect` is used only with documented port/baud and GUI permission.
- `protocol-check --wait-completion` returns a fresh complete report.
- `scan-params --wait-completion` settles at the expected parameter count.
- `param-get` succeeds for a documented parameter.
- bounded RX logs show expected responses.
- wave summary appears when telemetry is expected.

Do not bypass GUI permissions by opening the serial port directly from another process.

## 7. Handoff evidence

Report:

- protocol files and active protocol facts used
- firmware files changed
- parameter names and access levels added/changed
- wave channels added/changed
- build/test commands and exact results
- whether flashing was run
- whether GUI protocol-check/scan/get was run
- any skipped checks, remaining safety assumptions, or protocol conflicts
