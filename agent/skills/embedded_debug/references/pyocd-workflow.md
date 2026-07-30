# pyOCD Workflow

Chinese version: [pyocd-workflow_zh.md](pyocd-workflow_zh.md).

## Discover the tool and target

Prefer the project-documented entry point. On Windows, when `pyocd.exe` is not
on PATH but the module is installed, use:

```powershell
python -m pyocd --version
python -m pyocd list
python -m pyocd list --targets
```

A zero exit code from probe enumeration is not enough; require a probe matching
the intended UID. Do not choose automatically when multiple probes are listed.

Do not assume the generic `cortex_m` target supports device Flash, full memory
maps, or peripheral descriptions. Select a target verified for the requested
operation and installed pyOCD version.

## Parameterized connection

Build commands from discovered values:

```text
--target <target> --uid <probe-uid> --frequency <swd-frequency>
```

Use an SWD/JTAG frequency supported by the wiring and target. Lower frequency
may help signal-integrity or connection problems, but is not a substitute for
correct target and probe selection.

## Reset

Reset changes target execution state and destroys volatile fault context. Run
it only when the request includes reset or the impact has been accepted.

```powershell
python -m pyocd reset `
  --target <target> `
  --uid <probe-uid> `
  --frequency <swd-frequency> `
  --method sw `
  --no-wait
```

Prefer `sw` or `sysresetreq` unless project evidence supports another method.
Use `hw` only when NRST is physically wired. `--no-wait` controls waiting for a
probe connection; it does not mean the MCU reset itself is asynchronous.

## Short register snapshot

Use one-shot commander commands and end with `go`:

```powershell
python -m pyocd commander `
  --target <target> `
  --uid <probe-uid> `
  --frequency <swd-frequency> `
  --elf '<firmware.elf-or-axf>' `
  -c halt `
  -c 'reg pc' `
  -c 'reg lr' `
  -c 'reg sp' `
  -c 'reg xpsr' `
  -c 'reg basepri' `
  -c 'reg primask' `
  -c go
```

The commander resume command is `go`. If a command fails before `go`, run a
separate verified recovery command and report that the target may remain halted
until recovery is confirmed.

## Read memory only after address validation

Find a variable or register address from the matching ELF/AXF, map, SVD, or
device manual. Then use the correct width:

```powershell
python -m pyocd commander <connection-arguments> `
  -c halt `
  -c 'read32 <confirmed-address>' `
  -c go
```

Check whether a peripheral register is read-to-clear or otherwise has read side
effects. Do not write memory or registers without explicit purpose and
authorization.

## Validate the result

- Record the exact command and exit code.
- Confirm the selected probe and target in output.
- Require all requested register values.
- Confirm `go` succeeded after a snapshot.
- After reset, verify startup markers and continuing heartbeat separately.
- Do not equate successful pyOCD connection/reset with healthy firmware.

Close Keil Debug or another session if it owns the same probe.
