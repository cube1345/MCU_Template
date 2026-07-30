# Keil uVision Workflows

Chinese version: [workflows_zh.md](workflows_zh.md).

## Action selection

| User intent | Action | Hardware effect |
| --- | --- | --- |
| Build, compile, check errors, verify firmware | Build | None |
| Flash, download, program the board | Flash | Rewrites target flash |
| Build and flash, compile and download | Build then flash | Rewrites target flash after build |
| Run | Inspect project documentation | Ambiguous; do not assume |

## Common UV4 command forms

Use quoted, project-specific paths:

```text
"<UV4.exe>" -b "<project.uvprojx>" -j0 -o "<build-log>"
"<UV4.exe>" -f "<project.uvprojx>" -j0 -o "<flash-log>"
```

- `-b` builds the configured target.
- `-f` invokes the project's configured flash/download workflow.
- `-j0` requests available parallel build capacity.
- `-o` writes the uVision output to the chosen log file.

Verify these options against the installed uVision version and project scripts.
Do not copy paths, targets, or output names from an unrelated project.

## Build

1. Read the candidate script before running it.
2. Confirm it invokes only build behavior, such as UV4 `-b`, and does not
   contain UV4 `-f`, call a flash script, or launch a build-and-flash wrapper.
3. Capture the pre-build artifact state.
4. Run the verified build-only script or raw build command.
5. Capture the exit code immediately.
6. Require the build log to exist.
7. Parse and report errors and warnings.
8. Confirm the expected output exists and was refreshed when a rebuild was
   expected.

Do not convert warnings into silent success. A warning build may still produce
an artifact, but report it distinctly.

## Flash

1. Complete the flash preflight and authorization gate.
2. Confirm the expected artifact exists and identify its timestamp and size.
3. Run the project flash script or raw verified command.
4. Capture the exit code immediately.
5. Require the flash log to exist.
6. Check erase, programming, and verification status.

Flash success does not prove that the application works.

## Build and flash

Run build and flash as two explicit stages:

```text
build
  ├─ failure, missing log, errors, or invalid artifact -> stop
  └─ verified artifact -> flash
       ├─ failure -> report and retain logs
       └─ verified programming -> report hardware write complete
```

Never continue to flash because an old AXF happens to exist after the new build
failed.

## Script naming

A project script named `run_keil.bat` may mean build-and-flash rather than an
interactive debug/run session. Read its implementation before using it and
describe what it actually does. Never use such a script for a build-only request
if its body includes flashing.
