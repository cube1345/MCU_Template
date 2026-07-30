# 工程内串口诊断命令参考

The GUI owns the serial port. The helper appends UTF-8 JSONL commands to the chosen runtime and waits for the matching result `id`.

## Runtime selection and files

Runtime precedence is:

1. explicit `--runtime`
2. source tree `Serial_Monitor/runtime/`
3. `runtime/` below a root containing `SerialMonitor.exe`
4. a root explicitly recognizable as the runtime itself
5. project fallback `<project>/agent/serial/`

The GUI and helper must use the same selected runtime. The GUI processes only commands appended after it starts, so submit a fresh `status` request after launch. Discovery reports a candidate; it does not prove a currently running GUI uses that directory.

| Context | Runtime |
| --- | --- |
| This source repository | `Serial_Monitor/runtime/` |
| Packaged EXE quick mode | `<exe-directory>/runtime/` |
| GUI-selected or documented runtime | pass its exact path with `--runtime` |
| Hardware project fallback | `<project>/agent/serial/` |

Within the runtime, the queue is `ai_commands.jsonl`, results are `ai_results.jsonl`, the current wave summary is `ai_wave_summary.json`, the active protocol is `protocol.json`, and protocol reports are under `protocol_reports/`.

## Permission mapping

Humans grant permissions only in `Settings > AI Permissions`; queue entries cannot grant them.

| Action | Required GUI permission |
| --- | --- |
| `status` | none; reports current permissions even if master is off |
| `refresh_ports` | master bridge enabled |
| `read_serial_log`, `scan_params`, `param_get`, `protocol_check` | Read device data |
| `connect`, `disconnect` | Control serial connection |

`ok:false` plus `data.required_permission` is a valid, safe result. Do not bypass it.

## Actions exposed by the project helper

```jsonl
{"id":"status","type":"gui","action":"status"}
{"id":"ports","type":"gui","action":"refresh_ports"}
{"id":"log","type":"gui","action":"read_serial_log","lines":100,"direction":"rx"}
{"id":"errors","type":"gui","action":"read_serial_log","lines":100,"direction":"rx","contains":"error"}
{"id":"connect","type":"gui","action":"connect","port":"COM3","baud":115200}
{"id":"disconnect","type":"gui","action":"disconnect"}
{"id":"protocol","type":"gui","action":"protocol_check"}
{"id":"scan","type":"gui","action":"scan_params"}
{"id":"get-target","type":"gui","action":"param_get","name":"target"}
```

The helper validates `read-log --lines` as 1–500. Directions are `all`, `rx`, `tx`, `ai`, and `system`; `contains` filters case-insensitively. It requires explicit `connect --port <port> --baud <positive-int>`. Automatic selection is allowed only as `--port auto --port-hint <documented-hint>`.

## Asynchronous completion

- `--timeout 15 protocol-check --wait-completion` starts a check and waits for a new complete report.
- `latest-protocol-report` reads the newest complete report without starting work.
- `wait-protocol-report --after <epoch-seconds>` waits for a complete report newer than the supplied boundary. When following an already-started check, use that check's matched result `time_unix`; without this boundary, an old report cannot prove current completion.
- `scan-params --wait-completion` starts a scan and waits for a positive parameter count to remain stable across status polls.
- `wait-scan --minimum-count <n> --stable-polls <n>` waits separately.

A connection response means only request acceptance; query `status`. A settled scan proves the GUI parameter count stopped changing, not that every value is correct. A protocol check is complete only when a new valid report is read.

`summary --max-age <seconds>` rejects missing, malformed, non-object, timestamp-free, or stale summaries. JSONL result reads retain incomplete final lines for the next poll and recover if the result file is truncated.

The bundled helper intentionally omits actions that send or alter device data. Refer to root `AI_COMMANDS.md` only when a separately authorized task requires the complete contract.
