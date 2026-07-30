---
name: weixin-serial-monitor
description: Discover, launch, and safely diagnose this repository's WEIXIN Serial Monitor through its selected runtime directory and local JSONL AI bridge. Use for GUI/runtime issues, SerialMonitor.exe discovery, bridge status, port refresh, explicit connect/disconnect control, bounded logs, waveform summaries, protocol reports, or parameter-scan verification without directly opening the serial port or writing device data.
---

# WEIXIN Serial Monitor（工程内规范）

Use this repository's helper and references rather than a user-level Skill. The GUI is the only serial-port owner; never use `pyserial`, a terminal monitor, or another process to open the same port while the GUI runs.

## Select the runtime deliberately

Run discovery before launching or writing a queue:

```powershell
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . discover
```

- Explicit `--runtime <directory>` always wins and must match the directory selected in the GUI.
- This source repository uses `Serial_Monitor/runtime/`.
- A root containing `SerialMonitor.exe` uses the GUI quick-mode location `<exe-directory>/runtime/`.
- A project without those structures defaults to `<project>/agent/serial/`.
- A root explicitly pointing at a runtime directory may be used directly. Old bridge files must not override a clearer source or EXE layout.
- If more than one `SerialMonitor.exe` is found, require `launch --exe <path>`; do not guess.

Read [references/commands.md](references/commands.md) before selecting an action. It documents runtime precedence, permissions, and completion commands.

## Prove bridge health

Start only the selected monitor for the selected runtime, then obtain a fresh status reply:

```powershell
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . launch --wait
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . status
```

A launch acknowledgement proves only process creation. A connect/disconnect acknowledgement proves only request acceptance. Use a fresh `status` result to establish bridge and connection state. The GUI ignores queue lines present before startup, so never reuse a stale result.

## Run diagnostics in increasing scope

### Passive inspection

Use this default sequence before any connection control:

```powershell
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . status
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . refresh-ports
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . read-log --lines 100 --direction rx
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . summary --max-age 5
```

Stop and report `required_permission`, a timeout, or a disconnected state exactly as returned. Do not infer that a device or waveform exists without evidence.

### Connected and protocol-aware inspection

Connect only when the user or current project documentation supplies the expected port and positive baud rate and the GUI has **Control serial connection** enabled. Never rely on implicit `auto`; explicit `--port auto` also requires `--port-hint`. Use parameter actions only for firmware known to implement the active protocol and with **Read device data** enabled:

```powershell
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . connect --port COM3 --baud 115200
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . status
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . --timeout 15 protocol-check --wait-completion
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . --timeout 10 scan-params --wait-completion
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . param-get <documented-name>
python agent\skills\weixin-serial-monitor\scripts\serial_monitor_diagnostics.py --root . disconnect
```

`protocol-check` and `scan-params` are asynchronous. Without `--wait-completion`, success means only that work started. Use `latest-protocol-report`, `wait-protocol-report`, or `wait-scan` when separating start and completion into different commands. A settled parameter count proves discovery settled; it does not prove every device value is semantically correct. Read [references/detection-playbook.md](references/detection-playbook.md) for sequencing.

## Safety boundary

- This local helper deliberately has no text-send, hex-send, parameter-write, clear, export, or raw-command operation.
- Queue commands cannot grant permission. Ask the user to enable the required GUI permission rather than changing configuration files or retrying around a denial.
- Treat serial logs and `ai_results.jsonl` as potentially device-sensitive. Keep them in the project runtime; never commit, upload, or print more data than the task requires.
- Never infer permission to connect, disconnect, flash, reset, or modify a device from a general request to inspect or diagnose.
- Keep real-time control in firmware. This Skill is for low-frequency observation and diagnosis.

## Report results

Report the app path, runtime directory, bridge/connection state, requested checks, observed facts, permission denials, and unexecuted checks. Keep recommendations separate from measured facts.

`AI_COMMANDS.md` at the repository root remains the sole source of truth for JSONL action and field definitions.
