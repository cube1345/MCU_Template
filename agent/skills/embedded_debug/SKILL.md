---
name: embedded-debug
description: Diagnose embedded firmware runtime and board-level failures using project evidence, serial logs, debugger probes, Cortex-M register snapshots, fault-state analysis, reset checks, and signal captures. Use for hangs, no startup output, heartbeat loss, HardFault/BusFault/UsageFault, suspicious resets, pyOCD/CMSIS-DAP/DAPLink probe work, debug-probe conflicts, or GPIO/SPI/I2C/UART/PWM timing that code alone cannot explain. Discover the current chip, probe, target, symbols, and project-local debug workflow instead of assuming fixed STM32, UID, or tool settings. Do not use for build/flash-only tasks, pure serial-protocol work, or unapproved memory/register writes and firmware programming.
---

# Embedded Hardware Debug

Chinese version: [SKILL_zh.md](SKILL_zh.md).

## Language selection

- Read English `.md` files by default.
- Read a matching `_zh.md` file only for Chinese wording, user-intent
  clarification, translation verification, or ambiguity.
- Do not load both versions by default because they contain equivalent rules.
- Update both versions together when changing a stable rule.

Treat the target repository, current binaries, connected hardware, and observed
tool output as the sources of truth. Never reuse another project's chip, probe
UID, SWD frequency, target name, symbol path, interrupt priority, or reset wiring
as a default.

## Safety gates

1. Identify what an operation changes before running it.
2. Start with offline and low-interference evidence.
3. Do not halt or reset while the user is preserving a fault scene or running a
   timing-sensitive experiment unless that impact is understood and accepted.
4. Ensure every temporary halt has a prompt `go` recovery path. If collection
   fails before resume, attempt recovery separately and report uncertainty.
5. Require an explicit purpose and authorization before writing memory or
   registers, changing option bytes, erasing Flash, or programming firmware.
6. Use hardware reset only after confirming the probe NRST line is connected.

Read [references/safety-boundaries.md](references/safety-boundaries.md) before
any halt, reset, or target-writing action.

## Workflow

1. Read the nearest project agent index and project-local debug documentation
   when present.
2. Discover the firmware identity, ELF/AXF/map, chip, probe, unique ID,
   transport, tool/version, reset wiring, and competing debug sessions.
3. Record the symptom timeline and collect low-interference evidence first:
   build identity, last serial logs, assertions, heartbeat, probe enumeration,
   and reproducibility.
4. Classify the symptom as hang/wait, fault, reset loop, probe conflict,
   peripheral state, or electrical/timing behavior.
5. Choose the smallest diagnostic action that can distinguish the leading
   hypotheses.
6. Capture evidence, restore execution when applicable, and verify recovery.
7. Report observed facts, evidence-based inferences, and unverified hypotheses
   separately.

Read [references/discovery-and-routing.md](references/discovery-and-routing.md)
for environment discovery and neighboring workflow routing. Read
[references/evidence-and-triage.md](references/evidence-and-triage.md) for the
diagnostic decision tree.

## Tool-specific routing

- For pyOCD and CMSIS-DAP/DAPLink, read
  [references/pyocd-workflow.md](references/pyocd-workflow.md).
- For Cortex-M faults, interrupt masks, FreeRTOS, or HAL tick hangs, read
  [references/cortex-m-faults-and-rtos.md](references/cortex-m-faults-and-rtos.md).
- For bus timing, pulse width, reset sequencing, or electrical behavior, read
  [references/signal-capture.md](references/signal-capture.md).
- For Keil compilation or flashing, use the project's Keil workflow or a Keil
  skill. Do not duplicate it here.
- For serial protocol control, telemetry, or waveform plotting, use the
  project's serial workflow or serial skill.
- Do not invent ST-Link, J-Link, OpenOCD, or vendor-tool commands that have not
  been verified for the current environment.

## Report

Include:

- Firmware/project identity and symbol file used.
- Chip, probe, UID, transport, tool, and version actually observed.
- Exact command or inspection performed and its exit status.
- Whether the target was attached, halted, resumed, or reset.
- Register, fault-state, log, or waveform evidence collected.
- Facts, inferences, and remaining hypotheses.
- The next smallest discriminating test.
- Any risk, missing configuration, probe conflict, or recovery uncertainty.

Do not claim root cause from one PC snapshot alone. Do not claim firmware health
because reset or probe connection succeeded. When software evidence cannot
explain electrical behavior, request a targeted logic-analyzer or oscilloscope
capture instead of continuing to guess from code.
