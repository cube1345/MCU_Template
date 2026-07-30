---
name: vscode-c-navigation
description: Diagnose, repair, and verify Visual Studio Code declaration, definition, reference, and header navigation for C/C++ projects, especially Keil, STM32, CMSIS, HAL, FreeRTOS, and other embedded projects. Use when F12, Ctrl+Click, Go to Declaration, Go to Definition, Find All References, semantic completion, or header navigation does not work; when IntelliSense is inactive or uses the wrong macros/includes/compiler; or when a Keil-only project needs a safe CMake-generated compile_commands.json without changing its real firmware build workflow.
---

# VSCode C Navigation

Chinese version: [SKILL_zh.md](SKILL_zh.md).

## Language selection

- Read this English file by default.
- Read `SKILL_zh.md` only when Chinese wording, translation verification, or
  ambiguity requires it.
- Keep both files semantically synchronized whenever changing this skill.

Treat the target repository, its agent instructions, and its real build project
as the source of truth. A reference repository is evidence, not a template to
copy blindly.

## Diagnose in this order

Do not begin by repeatedly changing `includePath`. First establish whether a
language service is running at all.

1. Identify the exact VSCode window and workspace root. Use `code --status`,
   process command lines, or the visible window title. Do not diagnose a second
   welcome window or another copy of the repository.
2. Confirm the active file language mode is `C` or `C++`.
3. Confirm `ms-vscode.cpptools` is installed **and enabled for this workspace**.
   An installed extension can still show `Enable (Workspace)` on its details
   page. Enable it before editing project configuration.
4. Check workspace trust. If VSCode asks whether to trust the folder, obtain
   any confirmation required by the active UI-automation policy before choosing
   `Trust Folder and Continue`. Do not bypass a trust prompt.
5. Inspect user and workspace settings for:
   - `C_Cpp.intelliSenseEngine: "disabled"`;
   - a stale `configurationProvider`;
   - a missing or stale `compileCommands` path;
   - conflicting `clangd`, EIDE, PlatformIO, or other language servers.
6. Inspect the current extension-host and C/C++ output logs. Verify that
   `ms-vscode.cpptools` activates and that `cpptools.exe` or its server starts.
   The absence of activation is stronger evidence than a valid JSON file.

Use `C/C++: Log Diagnostics` when available. If the command is absent from the
command palette, treat that as evidence that the extension is disabled,
untrusted, not registered in that window, or not activated.

## Select the configuration strategy

Prefer a real compilation database for nontrivial projects.

### Existing compilation database

If the real build already exports `compile_commands.json`, point C/C++ directly
to it. Do not create a parallel build description.

### Keil-only or vendor-IDE embedded project

Create an index-only `CMakeLists.txt` when the project has no compilation
database:

- Keep Keil or the vendor IDE as the real build, link, download, and debug flow.
- Use CMake only to generate `compile_commands.json`.
- Mirror the active project sources, include directories, preprocessor defines,
  language standard, MCU, FPU, and ABI from the real project.
- Prefer active source lists over globbing every vendor driver module.
- Use `CMAKE_SYSTEM_NAME Generic`,
  `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`, and
  `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
- Use an available ARM GCC/Clang toolchain for indexing. Discover its path;
  never assume a path copied from another machine.
- Use forward slashes in CMake command-line paths on Windows. If a failed cache
  contains escaped backslashes, rerun CMake with `--fresh`.

An ARMCC-specific FreeRTOS port or inline-assembly file may fail an optional GCC
index build while the database remains correct. Do not modify production
firmware merely to make the index-only target compile. Configuration success
and database correctness are the navigation criteria.

### Direct C/C++ configuration

Prefer direct consumption over provider coupling when activation order is
unreliable:

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/cmake-build-vscode/compile_commands.json",
  "C_Cpp.intelliSenseEngine": "default"
}
```

Add `.vscode/c_cpp_properties.json` when it is useful to:

- trigger C/C++ workspace activation;
- declare `compileCommands`, `compilerPath`, and `intelliSenseMode` directly;
- provide fallback `defines`, `includePath`, and `browse.path` values.

Do not keep multiple contradictory compiler paths, compile databases, or
configuration providers. A direct `compileCommands` configuration should not
wait on an inactive CMake provider.

## Implement safely

1. Read repository agent files before editing.
2. Preserve user changes and the real IDE project.
3. Add only the minimum index files normally required:
   - `CMakeLists.txt` for index-only projects;
   - `.vscode/settings.json`;
   - `.vscode/c_cpp_properties.json` when direct activation or fallbacks help;
   - `.vscode/tasks.json` for database regeneration;
   - `.vscode/extensions.json` for recommendations;
   - a generated-build entry in `.gitignore`.
4. Generate the database immediately. Do not hand off an unconfigured CMake
   file.
5. Avoid unsupported CLI guesses such as `code --enable-extension`; enable or
   disable an extension through supported VSCode UI/commands.

## Validate with evidence

Complete all applicable checks:

1. Parse every edited JSON file.
2. Configure CMake with exit code zero.
3. Parse `compile_commands.json` and verify entries for representative files
   from application, platform, driver, RTOS, and vendor HAL layers.
4. Inspect one command entry for the expected compiler, MCU flags, defines, and
   include directories.
5. Reload the intended VSCode window.
6. Verify C/C++ extension activation in logs and verify the language-server
   process starts.
7. Run `C/C++: Log Diagnostics` and confirm the intended configuration and
   compilation database are selected.
8. Test F12 or Ctrl+Click on a known project symbol in both directions:
   declaration to definition and call site to declaration/definition.
9. Test one framework symbol such as a HAL or RTOS API when applicable.

Do not report success solely because `compile_commands.json` exists. Navigation
is fixed only after the active VSCode window consumes the configuration and a
real symbol jump succeeds.

## Report

State:

- the actual root cause;
- which files or VSCode states changed;
- whether workspace trust or extension enablement was required;
- compilation database entry count and representative coverage;
- the exact symbols used for the final navigation test;
- any index-only compiler incompatibility that does not affect the real build.

