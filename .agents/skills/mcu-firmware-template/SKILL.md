---
name: mcu-firmware-template
description: Project-level development rules for this portable layered MCU firmware template. Use when implementing, reviewing, porting, configuring, building, flashing, or debugging firmware in this repository.
---

# Portable MCU Template Development Skill

## Scope

This repository is currently one STM32 HAL/CMSIS example profile using C11, CMake, and Ninja. The architecture is intended to support multiple MCU families, part numbers, packages, boards, and toolchains. The firmware is organized as:

```text
Application / 6_apps / 5_libs
              |
              v
          4_facade
              |
              v
           3_board
              |
              v
           2_device
              |
              v
       1_core + 0_hal
              |
              v
       STM32 HAL / CMSIS / MCU
```

The default branch/profile is a reusable template. A chip profile identifies MCU and toolchain facts; a Board profile identifies PCB resources. Neither should be confused with reusable Device algorithms.

## First Read

Before changing code, read the relevant files in this order:

1. `docs/myused_five_layer_architecture.md`
2. `docs/command_line_build_flash_debug.md`
3. `CMakeLists.txt`
4. `cmake/stm32cubemx/CMakeLists.txt`
5. `CMakePresets.json`
6. `Template.ioc`
7. The target module's `.h`, `.c`, Board file, and Facade file

Also inspect `git status --short` before editing. Preserve user changes and do not edit generated build output.

## Architecture Contract

### `0_hal`

`0_hal` is the only project-owned hardware abstraction boundary.

- Wrap STM32 HAL, CubeMX handles, registers, GPIO, I2C, UART, and timer operations here.
- Public project HAL functions use the `HAL_` prefix.
- Keep APIs available when a CubeMX module is disabled. Return an error, safe default, or no-op according to the existing module pattern.
- Do not put device meaning in HAL. HAL may know GPIO and timer concepts, but must not know that a pin is an LED or motor enable.
- Do not scatter direct vendor calls through Device, Board, Facade, or App code unless an existing module explicitly requires it and the boundary is being repaired.

### `1_core`

Core contains hardware-independent rules only.

- Objectized devices contain `device_base_t base`.
- Use existing category ops from `device_ops.h` when the capability is shared by multiple devices.
- Keep device-specific operations in the device API; do not turn Core into a universal API.
- Core must not include Device, Board, Facade, CubeMX, or application headers.

### `2_device`

Device implements what a component is and what it can do.

Typical shape:

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    /* instance configuration */
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
    /* instance runtime state */
};
```

- Put per-instance parameters in `cfg` or the object runtime state.
- Inject hardware through configuration structures, callbacks, or ops.
- Keep algorithms and device state here.
- Do not include `3_board`, `4_facade`, `5_libs`, or `6_apps` from Device code.
- Private variables, ops tables, default configurations, and helpers must be `static`; read-only tables/configurations should be `static const`.
- Public functions must validate null pointers and uninitialized objects consistently with surrounding code.

### `3_board`

Board binds a generic Device object to one concrete board.

- Instantiate device objects here.
- Bind GPIO ports/pins, timer instances/channels, I2C/SPI/UART handles, device addresses, polarity, and interrupt resources here.
- Put board-specific ISR entry points here when the resource is board-specific.
- Keep algorithms out of Board. For example, encoder decoding belongs in Device; speed control belongs in App.
- The default template should remain safe and buildable without real board bindings. Return null objects, zero addresses, or disabled status where no board resource exists.
- A concrete board profile may bind resources, but must not silently change the default template to a private PCB mapping.

### `4_facade`

Facade is the stable business-facing entry point.

- Hide Board objects and ops dispatch when that reduces application coupling.
- Preserve legacy APIs where they are already part of the project contract.
- Add a wrapper only when it hides an object, simplifies parameters, combines operations, or preserves compatibility.
- Facade must not access MCU registers or vendor HAL directly.
- Header-only forwarding is acceptable for modules that need no additional policy.

### `5_libs` and `6_apps`

- Libraries provide reusable algorithms and utilities; they must depend downward only through Facade, Device APIs, or injected callbacks.
- Apps are examples and integration logic. They may combine multiple Facades/Devices.
- The template App code must remain compilable but should not run by default unless explicitly enabled in `main` or a build profile.
- Do not make HAL, Core, Device, or Board depend on App code.

## Include Direction

Use this dependency direction:

```text
App / Libs -> Facade -> Board -> Device -> Core / HAL -> STM32 HAL / CMSIS
```

Preferred includes:

- App/Lib: `4_facade/*.h` and public library headers.
- Board: `2_device/*.h`, `0_hal/*.h`, and required CubeMX board headers.
- Device: `1_core/*.h` and `0_hal/*.h`.
- Core: standard types only and Core headers.
- HAL: STM32 HAL/CMSIS headers and standard types.

Avoid relative include paths that bypass ownership boundaries. If an include is required only by implementation, keep it in `.c` rather than `.h`.

## Adding a Peripheral

Follow this sequence:

1. Confirm the electrical interface, voltage, polarity, timing, reset behavior, and interrupt requirements from the component datasheet and the target MCU reference manual.
2. Check whether an existing ops category fits. Extend `1_core/device_ops.h` only for a stable capability shared by a device class.
3. Add the Device object, config, runtime state, ops table, initialization, and public methods in `2_device`.
4. Inject bus and GPIO operations through config/callbacks. Do not include Board from Device.
5. Add a Board object and concrete resource binding in `3_board`. Keep the default profile safe if the resource is absent.
6. Add only useful business or compatibility calls in `4_facade`.
7. Add source files to the CMake source set. This project currently uses `GLOB CONFIGURE_DEPENDS` for `.c` files in `0_hal` through `6_apps`; verify the generated build sees the new file.
8. Update the relevant docs when the public API, board profile, build flow, or migration behavior changes.
9. Build Debug and inspect warnings/errors. Check the map/memory report when adding buffers, displays, or large lookup tables.

## GPIO and Interrupt Rules

- Let CubeMX generate peripheral clock enable and pin initialization when the pin is part of the CubeMX project.
- Board code may bind a generated `GPIO_TypeDef *` and pin mask to a Device object.
- Set initial output levels before switching a pin to output when that avoids glitches.
- Document active-high/active-low behavior in the config, not in scattered call sites.
- In ISRs, clear the source, capture minimal state, and defer expensive work to the main loop or task.
- Do not perform blocking I2C, display rendering, logging, dynamic allocation, or long calculations in an ISR unless the design explicitly proves it safe.
- Use `volatile` only for state shared with an ISR or hardware; protect multi-byte shared state when the MCU can observe partial updates.

## Resource and Runtime Rules

- Avoid dynamic allocation in the firmware template unless a concrete runtime policy exists.
- Avoid writable global device state. Use object state and controlled getters/setters.
- Use fixed-width integer types for hardware-facing values.
- Check arithmetic width, signedness, overflow, unit conversion, and timer prescaler limits.
- Treat time values explicitly as milliseconds, microseconds, ticks, radians, degrees, raw counts, or scaled units.
- Keep blocking delays out of reusable drivers. Put scheduling policy in App or the integration layer.
- For error handling, return the project/module status type where one exists; otherwise preserve the established safe no-op behavior and expose enough state for diagnosis.

## Portable MCU and Board Migration

Read [references/migration.md](references/migration.md) before changing a chip profile, Board profile, startup, linker, or vendor HAL.

This repository contains one vendor-specific example profile. Treat its MCU name, package, memory sizes, startup file, linker script, device header, peripheral macros, compiler flags, and probe target as profile data. They are not universal architecture rules.

When porting, verify the target datasheet, reference manual, errata, CMSIS/HAL release, package pinout, clock tree, memory map, peripheral availability, alternate functions, IRQs, DMA, and debug connection. Change the chip profile for toolchain and generated MCU files; change the Board profile for PCB resources; preserve Device algorithms and Facade contracts where their semantics remain valid.

Never port by globally replacing one part number or family name with another. Recheck every memory region, startup vector, clock limit, ABI/FPU setting, peripheral instance, interrupt, DMA mapping, and electrical assumption independently.
## Build, Flash, and Debug

From the project root:

```powershell
cmake --preset Debug
cmake --build build\Debug
```

Expected firmware artifact:

```text
build\Debug\Template.elf
```

For ST-Link or DAPLink, use the repository script after setting `OPENOCD_EXE` and `OPENOCD_SCRIPTS` when OpenOCD is not on `PATH`:

```powershell
.\scripts\openocd_probe_auto.ps1 -Command probe -Probe stlink
.\scripts\openocd_probe_auto.ps1 -Command flash -Probe stlink
.\scripts\openocd_probe_auto.ps1 -Command gdbserver -Probe stlink
```

If the probe detects but cannot read the target Core ID, inspect the target profile, SWD/JTAG wiring, ground, target voltage, reset wiring, boot configuration, probe ownership, and debug-pin loading before changing firmware.

## Verification Checklist

Before handing work to another agent or merging:

- Read the changed files and all public call sites.
- Confirm include direction and layer ownership.
- Confirm no private board pin or handle leaked into the default profile.
- Confirm new object state is instance-owned and private data is `static`.
- Run `cmake --preset Debug` after cache-affecting changes.
- Run `cmake --build build\Debug` after source or linker changes.
- Review warnings; do not hide new warnings with broad compiler flags.
- Check the generated ELF exists and the linker memory report is reasonable.
- If hardware is available, probe and flash, then validate reset behavior and the intended peripheral signal.
- Update documentation for changed API, pin binding, tool invocation, or portability assumptions.

## Handoff Format

When reporting work, include:

1. Changed files and the layer responsibility of each.
2. Behavioral result and hardware assumptions.
3. Commands run and whether they passed.
4. Remaining warnings, untested hardware paths, and known limitations.
5. Any required board-profile changes for the next engineer.


## Migration Routing

Use [references/migration.md](references/migration.md) for the step-by-step Board-to-Board, MCU-to-MCU, cross-vendor, profile-boundary, and acceptance procedures.

## Profile Boundary Rules

- Chip profile: CPU flags, device macro, CMSIS/startup/linker files, clock limits, HAL driver selection, toolchain paths, and build presets.
- Board profile: PCB pinout, external oscillator, power/reset wiring, peripheral handles, timer channels, addresses, polarity, callbacks, and ISR mapping.
- Device layer: protocol, state machine, conversion, control algorithm, and object lifecycle.
- Facade/App: product behavior and stable application calls.

A migration is complete only when the selected chip profile, Board profile, generated configuration, build system, documentation, and hardware test procedure agree.




