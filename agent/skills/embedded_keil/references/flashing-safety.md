# Flashing Safety

Chinese version: [flashing-safety_zh.md](flashing-safety_zh.md).

Flashing erases or rewrites nonvolatile memory on connected hardware. Keep the
action within the exact board and firmware scope authorized by the user.

## Authorization gate

Proceed when the user explicitly asks to flash, download, program, or perform a
documented build-and-flash workflow. Do not infer permission from requests such
as build, verify, test compilation, inspect logs, or check the code.

When a project script called `run` performs flashing, explain that behavior and
obtain authorization unless the user's request already clearly includes it.

## Target gate

Before execution, establish:

- Intended repository and `.uvprojx`.
- Intended target, MCU, and connected board.
- Expected firmware artifact and its freshness.
- Probe/programmer type.
- Flash algorithm and address range.

Stop if the connected target cannot be distinguished safely from another board
or if the configured algorithm does not match the MCU.

## Failure behavior

- Never flash after a failed or unverified build.
- Never substitute an older artifact silently.
- Preserve and report the relevant logs.
- Do not repeatedly retry hardware writes without diagnosing the failure.
- Do not expand from flashing into reset, erase-all, option-byte changes, or
  debug configuration changes unless those actions are separately in scope.

## Post-flash claims

Separate these outcomes:

1. Tool connected to the target.
2. Erase/program completed.
3. Verification passed.
4. Target started.
5. Firmware behavior was functionally verified.

Only claim the highest level actually supported by evidence.
