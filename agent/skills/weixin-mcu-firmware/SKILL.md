---
name: weixin-mcu-firmware
description: Write, adapt, review, or debug MCU firmware so it is compatible with this repository's WEIXIN Serial Monitor. Use when Codex is asked to implement lower-machine/下位机 firmware, UART protocol handlers, parameter list/get/set support, wave plotting output, STM32/Keil demo firmware changes, or firmware-side integration tests for the Serial Monitor protocol. Always use the current project protocol files instead of hardcoding protocol details from memory.
---

# WEIXIN MCU Firmware（工程内规范）

Use this Skill for firmware-side work. It complements `agent/skills/weixin-serial-monitor`, which is for launching the GUI and diagnosing through the AI bridge.

## Prime directive

Do not treat this Skill as the protocol source of truth. Before writing or reviewing firmware, read the current project protocol sources and follow the active protocol they define:

1. `Serial_Monitor/docs/mcu_uart_protocol.md` or `Serial_Monitor/docs/mcu_uart_protocol.zh.md`
2. `Serial_Monitor/src/serial_monitor/protocols/README.md`
3. `Serial_Monitor/src/serial_monitor/protocols/weixin_default.json`
4. The selected runtime's `<runtime>/protocol.json`, or another target-firmware protocol file explicitly named by the user
5. `protocol_adapter.py`, `param_protocol.py`, and `plot_protocol.py` when code-level verification of GUI behavior is needed
6. `AI_COMMANDS.md` only when the task also involves AI bridge behavior

Locate the runtime using the GUI selection or `agent/skills/weixin-serial-monitor/scripts/serial_monitor_diagnostics.py --root . discover`. The active configurable protocol is `<selected-runtime>/protocol.json`; it is not inherently beside the EXE. Prefer that active file for an existing project, the built-in `weixin_default.json` only when establishing the source default, and the docs for human-readable intent. If sources disagree, verify current code/config and report unresolved conflicts instead of guessing.

## Safety boundary

- Never open the serial port directly while WEIXIN Serial Monitor owns it.
- Treat flash/erase/program/unlock, reset or power-cycle, debugger attach/halt/run, bootloader entry, and option-byte/fuse/protection changes as separate hardware-changing or interrupting actions. Perform only the exact action explicitly authorized in the current task.
- Before an authorized hardware action, identify the board, image/target, tool, connection, power state, recovery path, and whether interruption is safe. Stop if target identity or recovery/isolation is unclear.
- Never add an AI-accessible firmware command that can move hardware, change power state, erase memory, disable protection, or create safety risk unless the user explicitly asks and the command is guarded in firmware.
- Keep real-time control loops in firmware. The GUI/AI bridge is for low-frequency observation, diagnostics, and slow tuning.
- Treat device logs, calibration values, waveforms, and parameter snapshots as device-sensitive.

## Workflow

### 1. Identify the target

Determine the MCU family, board, project type, toolchain, UART peripheral, baud rate, RTOS/bare-metal model, and whether the task is a new integration, bug fix, or protocol migration. If the user gave only a screenshot or vague goal, inspect the firmware tree before asking.

Read `references/firmware-implementation-guide.md` when implementing code. Read `references/validation-checklist.md` before compiling, flashing, or handing off.

### 2. Load the active protocol

Extract rather than memorize these facts from the current protocol files:

- command prefix, response prefix, encoding, and line ending
- required commands and command templates
- parameter row fields, type names, access flags, min/max rules, and list completion marker
- get/set success and error response forms
- plot series prefix and value separators
- protocol-check expectations and help markers

Use these facts in firmware code through named constants or a small protocol module. Avoid scattering protocol literals across unrelated application code.

### 3. Design a firmware-side adapter

Keep the UART protocol adapter small and replaceable:

- collect bytes in ISR/DMA/ring-buffer code
- parse complete lines in main loop or a low-priority task
- silently ignore non-protocol lines unless the current protocol requires otherwise
- map protocol parameter names to firmware variables through a table
- validate type, range, and read/write access before writes
- emit responses through one reporting function so line endings and encoding stay consistent

Do not block motor/control loops with printf-heavy parsing, file-style buffering, or slow AI-facing work.

### 4. Implement parameters deliberately

For each parameter, define:

- stable protocol name, independent of C symbol names
- type compatible with the current GUI protocol
- readable/writable access
- min/max values in engineering units
- safe default and persistence behavior if applicable
- side effects of changing it while the system is running

Prefer starting with a small safe parameter set: version/status values as read-only, and harmless tuning values as read/write. Do not expose destructive maintenance commands as ordinary writable parameters.

### 5. Implement wave output only when useful

Output plot data in the current protocol's accepted form. Group values that should share a sample timestamp on one line when the parser supports it. Rate-limit high-frequency telemetry so the UART, GUI, and logs remain usable.

### 6. Validate in layers

Use normal firmware checks first: compile, static warnings, and host/unit tests if the project has them. If flashing is authorized, run a cautious device smoke test with a reversible or safe parameter.

Then use `agent/skills/weixin-serial-monitor/SKILL.md` to verify through the GUI. GUI acknowledgements for protocol checks and scans are asynchronous, so use its bounded completion commands:

1. launch/discover the monitor with the selected runtime
2. connect only with documented port/baud and GUI permission
3. run `protocol-check --wait-completion`
4. run `scan-params --wait-completion`
5. read one known-safe parameter
6. observe bounded logs and waveform summary

Record the initial GUI connection state. Disconnect after verification only if this workflow created the connection; otherwise preserve the user's existing connection.

Report which checks were not run and why, especially when no real hardware, flasher, or serial port is available.

## What to edit

Prefer editing the target firmware project, not the Serial Monitor desktop code. Only edit GUI protocol files when the user explicitly asks to change the protocol itself.

When adapting an existing firmware, preserve its HAL, scheduler, naming style, warning policy, memory model, and board support layout. Add the smallest integration layer that satisfies the active protocol.

## Handoff

Summarize:

- protocol files read and active protocol facts used
- firmware files changed
- commands/parameters/wave channels implemented
- build, flash, and GUI verification results
- actions requiring explicit user/device approval
- remaining protocol gaps or safety assumptions
