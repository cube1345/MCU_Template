# MCU and Board Migration Reference

This reference is read when changing the target chip, PCB, toolchain, or vendor implementation. The main Skill defines layer ownership; this document defines the migration evidence and order.

## Profiles

Keep two explicit profiles:

- **Chip profile**: core and FPU, ABI flags, device macro, CMSIS headers, startup/vector table, linker memory regions, clock limits, HAL/LL sources, compiler and build preset.
- **Board profile**: package pinout, oscillator, power and reset wiring, debug wiring, GPIO polarity and pulls, peripheral handles, timer channels, DMA, IRQ routing, device addresses, and board callbacks.

A board change should normally change only Board bindings and generated configuration. A chip change may require `0_hal`, startup, linker, toolchain, and generated vendor sources. Device algorithms and Facade contracts remain stable where their semantics are unchanged.

## Board-to-Board Procedure

1. Record the old and new pin map, electrical levels, clocks, buses, timers, DMA, interrupts, addresses, and reset behavior.
2. Create a distinct Board profile or branch. Do not overwrite the default profile with private PCB constants.
3. Update the board configuration and regenerate generated code. Review the complete diff and preserve user sections.
4. Rebind object instances and callbacks in `3_board`; keep protocol and algorithms in `2_device`.
5. Verify alternate-function mapping, GPIO clock enables, initial output levels, pull resistors, active polarity, shared resources, IRQ priorities, timer clock calculation, and DMA conflicts.
6. Build, inspect compile commands and map output, connect with the selected probe, flash, reset, and test each peripheral signal.

## MCU-to-MCU Procedure

1. Read the target datasheet, reference manual, programming manual, errata, and vendor HAL/CMSIS release. Record core, memory, clock tree, vector layout, reset/watchdog behavior, peripheral instances, IRQ names, and DMA mapping.
2. Create a chip profile with the target compiler flags, device macro, CMSIS/startup files, linker script, HAL sources, and CMake preset. Do not infer memory size or FPU from a family name.
3. Regenerate configuration and compare clock initialization, pin alternate functions, interrupt handlers, DMA, enabled modules, and generated source lists.
4. Change `0_hal` only where the vendor API, register layout, timing, or capability differs. Preserve the project HAL contract and explicit unavailable-status behavior.
5. Change `3_board` for package and PCB resources. Keep `1_core`, `2_device`, and `4_facade` unchanged when their contracts still hold.
6. Check alignment, integer widths, endianness, cache/MPU, atomicity, tick and delay units, timer prescalers, watchdog, and low-power behavior.
7. Build Debug and Release, inspect warnings and memory usage, then run hardware smoke tests including reset and recovery.

For a cross-vendor port, implement a new `0_hal` adapter and vendor profile. Device code must not include vendor headers or branch on vendor macros. If a capability is unavailable, return a documented status or safe no-op.

## Current Repository Limits

The repository's current CMake files contain a concrete example profile: fixed device macro, startup source, linker script, CPU flags, and vendor driver paths. The OpenOCD helper also selects a target configuration from its probe options. These are implementation facts of the example, not the portable architecture. A future multi-target conversion should parameterize them through a chip profile or CMake toolchain/preset and make the target OpenOCD configuration an explicit profile input.

## Acceptance Evidence

A port is complete only when the selected chip and Board profiles agree with generated configuration, CMake sources and definitions, linker memory regions, documentation, and the hardware test procedure. Record the exact build preset, firmware artifact, probe/interface, adapter speed, reset method, and untested paths in the handoff.
