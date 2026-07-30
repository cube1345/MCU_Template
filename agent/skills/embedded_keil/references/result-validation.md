# Result Validation

Chinese version: [result-validation_zh.md](result-validation_zh.md).

Use independent evidence. A console message such as `Build finished` or
`Application running` is not sufficient by itself.

## Build decision

Check all three sources:

1. Exit code.
2. Build log.
3. Output artifact.

Common uVision command-line conventions use:

| Exit code | Typical meaning | Treatment |
| --- | --- | --- |
| `0` | Build completed without warnings or errors | Continue validation |
| `1` | Build completed with warnings | Report warnings and validate artifact |
| `2` | Build errors | Fail and do not flash |
| `3` | Fatal error | Fail and do not flash |

Treat the installed version and project wrapper as authoritative if they define
different semantics.

From the log, extract when available:

- Target and compiler/toolchain.
- Error count and warning count.
- Program size: Code, RO-data, RW-data, and ZI-data.
- Linker or pack failures.
- Elapsed time.

Require the expected AXF/HEX to exist. Record its full path, modification time,
and size. If a rebuild was expected but the timestamp did not advance, treat
the artifact as suspicious and do not flash until resolved.

## Flash decision

Require:

- Successful process/script exit code.
- A present flash log.
- Successful erase when erase is part of the workflow.
- Successful programming.
- Successful verification, such as `Verify OK`.

If any phase fails, report the first failure and do not describe the board as
successfully programmed.

`Application running` means that the tool released or started the target after
programming. It does not verify displays, serial protocols, peripherals,
control algorithms, or other application behavior.

## Missing or stale evidence

- Missing log: validation failed even if an artifact exists.
- Missing artifact: build failed for workflow purposes.
- Old artifact after failed build: never use it as evidence of success.
- Overwritten latest-log files: describe them as the latest run, not history.
- Concurrent runs sharing one log path: results are unreliable; use separate
  log paths or serialize the runs.
