# Project Discovery and Preflight

Chinese version: [project-discovery_zh.md](project-discovery_zh.md).

Use this checklist before running a Keil command. The target repository and its
checked-in documentation override examples from this skill.

## Read the local guidance

Look for the smallest relevant set:

1. Repository agent index or `AGENTS.md`.
2. `agent/keil/` documentation and scripts.
3. PowerShell rules when the repository provides them.
4. The intended `.uvprojx` and, for flashing, its `.uvoptx` configuration.

Do not infer that a script is valid merely because it exists.

## Build preflight

Confirm:

- The Keil executable exists, usually `UV4.exe`.
- The intended `.uvprojx` exists.
- The selected target exists when the project contains multiple targets.
- The device, compiler/toolchain, packs, and include paths are appropriate.
- The output directory and output name identify the expected AXF/HEX.
- The build log path is known and will not collide with another active build.
- The command does not depend on an incorrect current working directory.

Record the old artifact timestamp and size when an old artifact exists. This
makes it possible to detect a build that returned without refreshing output.

## Flash preflight

In addition to build checks, confirm:

- The user explicitly requested or approved flashing.
- The connected board and MCU are the intended target.
- The debug probe/programmer matches the project configuration.
- The configured flash algorithm matches the MCU and flash geometry.
- The flash base address and range are correct.
- The artifact is the intended build and is not merely an unrelated old file.
- Board power and debug connections are stable enough to proceed.

If any item cannot be established safely from the project, logs, connected
hardware, or user request, stop before flashing and ask for the smallest missing
piece of information.

## Configuration changes

If the project, device, output path, toolchain, algorithm, or probe changed:

1. Do not run the stale script.
2. Identify the current configuration from the project files.
3. Update or replace the project command only when that change is in scope.
4. Re-run preflight before execution.
