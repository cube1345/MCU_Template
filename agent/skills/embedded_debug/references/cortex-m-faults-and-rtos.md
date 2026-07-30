# Cortex-M Faults and RTOS Triage

Chinese version: [cortex-m-faults-and-rtos_zh.md](cortex-m-faults-and-rtos_zh.md).

Apply only to a verified Cortex-M target. Register availability differs across
cores; verify the architecture before requesting fault registers.

## Core snapshot meaning

- `PC`: sampled instruction address.
- `LR`: return address or exception-return encoding.
- `SP`, `MSP`, `PSP`: active and candidate stack locations.
- `xPSR`: execution state and active exception number.
- `PRIMASK`: global configurable interrupt mask.
- `BASEPRI`: priority threshold mask on cores that implement it.
- `FAULTMASK`: fault masking on cores that implement it.

Symbolize PC/LR with the exact ELF/AXF/map that produced the running firmware.
Check whether LR is an `EXC_RETURN` value before treating it as a normal code
address.

## Fault evidence

When supported by the core, inspect SCB fault status such as CFSR, HFSR, SHCSR,
MMFAR, BFAR, DFSR, and AFSR. Use the architecture and device documentation for
addresses and interpretation; Cortex-M0/M0+ do not expose the same configurable
fault set as M3/M4/M7/M33-class targets.

Capture the exception stack frame before reset. Determine whether MSP or PSP
was active from the exception-return state. Validate stack addresses against
the current linker map and RAM ranges.

## FreeRTOS and BASEPRI

On Cortex-M ports, FreeRTOS critical sections commonly use `BASEPRI` to mask
interrupts at or below the configured syscall-eligible urgency. The exact
encoded value depends on implemented priority bits and project configuration.

For a hang in `HAL_Delay()` or other tick wait:

1. Read PC, BASEPRI, PRIMASK, and the tick value when safely accessible.
2. Identify the actual HAL timebase source and IRQ priority.
3. Read the project's FreeRTOS syscall interrupt threshold.
4. Determine whether the timebase interrupt is masked in the observed critical
   section.
5. Inspect the timebase ISR before changing priority.

In the usual CMSIS numerical convention, a lower number means higher urgency.
An ISR that calls FreeRTOS `...FromISR()` APIs must satisfy the port's syscall
priority rules. An interrupt intentionally placed above the syscall threshold
must not call FreeRTOS APIs.

Do not copy fixed priorities from an example. Compute the relationship from the
current `FreeRTOSConfig`, NVIC priority grouping/implemented bits, HAL timebase
configuration, and ISR implementation.

## Interpretation discipline

- Nonzero BASEPRI near a stalled tick supports an interrupt-masking hypothesis;
  it does not prove it alone.
- Confirm whether the tick actually stops and whether the relevant IRQ is
  pending or masked.
- Check for other causes: disabled timer clock, stopped SysTick/timer, global
  interrupt mask, scheduler state, corrupted tick data, or blocking hardware.
- Prefer a fix that preserves RTOS interrupt-contract rules, then rebuild and
  reproduce the original scenario.
