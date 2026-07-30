# Safety Boundaries

Chinese version: [safety-boundaries_zh.md](safety-boundaries_zh.md).

## Impact levels

| Level | Examples | Default handling |
| --- | --- | --- |
| 0: Offline | Read source, map, ELF metadata, existing logs | Safe within task scope |
| 1: Enumerate | List tools, targets, USB probes | Low impact; verify selection |
| 2: Attach/read | Connect probe, read state without intentional halt | May perturb target; record impact |
| 3: Halt snapshot | Halt, read core state, `go` | Runtime disruption; keep brief and recover |
| 4: Reset | Software/system/hardware reset | Loses volatile state; require scoped intent |
| 5: Write/program | Write registers/memory, option bytes, erase/flash | Require explicit purpose and authorization |

## Halt discipline

- Explain that halt pauses RTOS scheduling, UART, display refresh, networking,
  timers, watchdog servicing, and other real-time behavior.
- Avoid halt during timing-sensitive or safety-relevant live tests unless the
  user accepts the interference.
- Prefer one-shot collection ending in `go`.
- If collection fails, attempt a separate resume command and report whether
  recovery was confirmed.
- Do not leave an interactive session halted without clear ownership.

## Reset discipline

- Preserve fault registers, stack frame, last logs, and reset-cause evidence
  before reset when they matter.
- Use hardware reset only with confirmed NRST wiring.
- Do not describe reset as read-only; it changes execution and RAM state.
- Verify startup and heartbeat after reset; reset command success alone is not
  functional validation.

## Address and register discipline

- Derive addresses from matching symbols, map, SVD, or device documentation.
- Check access width, alignment, privilege, and read side effects.
- Do not use an address copied from another build or MCU.
- Separate read access from writes. A request to inspect does not authorize a
  write.

## Shared tools and hardware

- Check whether Keil, pyOCD, OpenOCD, a vendor programmer, or another IDE owns
  the probe.
- Do not repeatedly reconnect when ownership is unclear.
- Ensure target identity before acting when multiple boards or probes are
  attached.
- Escalate electrical questions to waveform capture when debugger attachment
  itself may change the symptom.
