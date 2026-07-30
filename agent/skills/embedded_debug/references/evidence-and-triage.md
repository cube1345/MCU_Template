# Evidence and Triage

Chinese version: [evidence-and-triage_zh.md](evidence-and-triage_zh.md).

## Start with a timeline

Record:

- What changed before the failure.
- Whether the board reaches reset, startup, scheduler start, and application
  heartbeat.
- The last serial log, assertion, or status transition.
- Whether the problem is repeatable and what triggers it.
- Whether reset temporarily clears it.
- Whether attaching a debugger changes the symptom.

Confirm that the symbol file and source correspond to the running firmware.

## Classify the symptom

### No startup or reset loop

Check reset reason, power/reset behavior, startup logs, vector/entry state, and
whether the firmware image matches the board. Do not reset before preserving
the current evidence when reset cause matters.

### Hang or wait

Collect a short CPU snapshot: PC, LR, SP, xPSR, BASEPRI, and PRIMASK. Symbolize
PC/LR with the matching ELF/AXF/map. Decide whether execution is in normal code,
a busy wait, scheduler idle, lock wait, delay/tick wait, or a fault handler.

A single snapshot is a location sample, not proof of root cause. Repeat or add
targeted state only when needed.

### Fault

Capture core registers, the active stack pointer and exception frame, exception
status, and architecture-supported fault status registers. Preserve the first
fault evidence before reset. Use the matching symbol file to resolve addresses.

### Peripheral or global state

Confirm every address from symbols, map, SVD, or the device manual before
reading it. Beware read-to-clear and side-effect registers; a debugger read is
not always electrically or semantically passive.

### Probe conflict

Check for Keil Debug, another pyOCD/OpenOCD process, vendor programmer, or IDE
session using the same probe. Close or coordinate the owner instead of retrying
indefinitely.

### Timing or electrical behavior

If software state cannot distinguish the hypotheses, request a targeted logic
analyzer or oscilloscope capture. Do not infer bus timing from source code.

## Structure the conclusion

Report three categories:

1. **Observed facts**: direct logs, register values, waveform events, exits.
2. **Inference**: the explanation best supported by those facts.
3. **Unverified hypotheses**: plausible alternatives and the evidence needed to
   eliminate them.

End with the smallest next test that can separate the remaining hypotheses.
