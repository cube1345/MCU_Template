---
name: keil
description: Safely operate and diagnose Keil uVision firmware workflows on Windows. Use when Codex needs to build or compile a .uvprojx project, inspect Keil build logs, verify AXF or HEX artifacts, flash or download firmware, run a documented build-and-flash script, or troubleshoot UV4 command-line failures. Inspect every candidate script before selecting it; for build-only requests never run a script containing UV4 -f or another flash step. Prefer project-provided Keil configuration, and never flash hardware unless the user explicitly requests or approves it. Do not use for non-Keil builds or ordinary code explanation.
---

# Keil Workflow

Chinese version: [SKILL_zh.md](SKILL_zh.md).

## Language selection

- Read English `.md` files by default.
- Read a matching `_zh.md` file only when Chinese wording, user intent,
  translation verification, or an ambiguity in the English version makes it
  useful.
- Do not load both language versions by default; they contain the same rules
  and would duplicate context.
- When changing a stable rule, update both language versions in the same task
  so they remain equivalent.

Use the target repository as the source of truth. Do not assume that paths,
targets, devices, output names, flash algorithms, or probes from another project
apply.

## Hard safety gates

- Read a `.bat`, `.cmd`, or `.ps1` file before proposing or executing it.
- Determine behavior from its commands, never from a name such as `run`,
  `verify`, or `build`.
- For a build-only request, never select a script that contains UV4 `-f`, calls
  a flash/download script, or otherwise programs hardware.
- Never describe a script as build-only or read-only until its body has been
  inspected.
- Never flash without explicit user authorization.

Example: if a user asks only to verify compilation and mentions
`agent/keil/run_keil.bat`, inspect that file. If it performs build plus flash,
reject it for this request and select a verified build-only script such as
`build_keil.bat` or a verified UV4 `-b` command.

## Route the request

1. Classify the requested action:
   - **Build**: compile/link and verify artifacts without touching hardware.
   - **Flash**: program an existing, confirmed artifact to hardware.
   - **Build and flash**: build first, then flash only after a clean build.
   - **Debug or run session**: do not treat this as build-and-flash unless the
     project documentation explicitly defines it that way.
2. Default ambiguous requests such as "verify" or "check the firmware" to
   build only.
3. Treat an explicit request to flash, download, program, or build-and-flash as
   authorization for that scoped hardware action. Otherwise stop before flash
   and request approval.

## Discover the project workflow

1. Read the nearest project agent index when present, such as
   `agent/readme.md`, `agent/agent_readme.md`, or a Chinese counterpart.
2. Read the project's `agent/keil/` documentation and scripts when present.
3. Locate the intended `.uvprojx`, target, Keil executable, output artifact,
   and log paths. For flash, also inspect the device, probe/programmer, flash
   algorithm, and address range.
4. Prefer a verified project-provided script over constructing a new command.
5. Read [references/project-discovery.md](references/project-discovery.md) for
   the complete preflight checklist.

## Execute the smallest safe workflow

1. Inspect a candidate script before executing it. Classify it by the commands
   it actually runs, not by its filename or documentation label.
2. For a build-only request, run only a command or script that cannot flash.
   Reject any candidate containing a UV4 `-f` step or invoking another flash
   script. The mere presence of `run_keil.bat` never makes it appropriate for
   build verification.
3. For flash, confirm the artifact belongs to the intended project and is fresh
   enough for the request before touching hardware.
4. For build-and-flash, run the steps serially. Never flash after a failed
   build, a missing log, or a missing/stale artifact.
5. Capture the actual process exit code immediately. In PowerShell, save
   `$LASTEXITCODE` before running another native command.
6. Follow [references/workflows.md](references/workflows.md) for UV4 command
   forms, stop conditions, and action-specific checks.
7. Follow [references/windows-powershell.md](references/windows-powershell.md)
   for Windows invocation, quoting, encoding, and output handling.

## Verify independently

Do not accept a success banner alone. Verify:

- Process or script exit code.
- Build log error and warning counts, or flash log erase/program/verify status.
- Expected AXF/HEX existence, timestamp, and size.

Read [references/result-validation.md](references/result-validation.md) for
decision rules. Read [references/flashing-safety.md](references/flashing-safety.md)
before every hardware-writing action.

## Report

Report:

- Exact script or command executed.
- Project and target selected.
- Build error and warning counts.
- Artifact path, timestamp, and size when available.
- Whether flash was skipped, attempted, verified, or failed.
- The first blocking failure and the relevant log path.
- Any unverified hardware, toolchain, or project assumption.

Do not claim functional firmware success merely because Keil reports
`Application running`. That proves programming/startup status, not application
behavior. Use serial output, debugger state, or an explicit functional check
when the task requires runtime validation.

When a stable project-specific Keil path, target, workflow, or troubleshooting
rule changes, update the owning project documentation. Do not store transient
logs, one-off observations, or project-specific absolute paths in this skill.
