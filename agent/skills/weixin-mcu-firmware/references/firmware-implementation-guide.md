# Firmware Implementation Guide

Use this reference after reading `SKILL.md` and the current protocol files. It gives implementation shape, not fixed protocol values. Always replace examples with facts from the active protocol.

## Table of contents

1. Integration shape
2. UART receive path
3. Command parsing
4. Parameter table
5. List/get/set behavior
6. Error handling
7. Wave output
8. Persistence and runtime side effects
9. Porting notes

## 1. Integration shape

Create a small adapter layer such as `weixin_protocol.c/.h`, `serial_monitor_protocol.c/.h`, or an equivalent module matching the firmware style. Keep it between the board UART driver and the application variables.

Recommended public surface:

```c
void weixin_protocol_init(void);
void weixin_protocol_on_rx_byte(uint8_t byte);
void weixin_protocol_poll(void);
void weixin_protocol_report_wave(void);
```

Use the firmware project's existing UART transmit function. If there is no non-blocking transmit path, keep responses short and avoid printing inside high-priority interrupts.

## 2. UART receive path

Prefer this split:

- ISR/DMA callback moves bytes into a ring buffer or line buffer.
- Main loop/RTOS task extracts complete lines.
- Protocol parser handles complete NUL-terminated lines.

Extract line endings, encoding, maximum command shape, and buffer requirements from the active protocol and target firmware. Size the buffer from the longest accepted command plus terminator and NUL; do not copy a historical demo size or encoding into another target without verification.

Do not parse tokens, search parameter tables, or call `printf` from an ISR unless the existing project already accepts that cost.

## 3. Command parsing

Use exact token matching for the command prefix. The default protocol uses a prefix such as `param`, but do not hardcode it before reading the active files.

Parser expectations:

- Trim line endings.
- Split by spaces and tabs unless the active protocol says otherwise.
- Ignore empty lines.
- Ignore non-protocol lines silently when sharing a debug UART.
- Enforce the argument count for each command.
- Return protocol errors only for malformed lines that actually target this protocol.

Avoid dynamic allocation. A fixed `argv[]` over the line buffer is usually enough.

## 4. Parameter table

Represent firmware parameters with a static table. The table should be the only place where GUI-visible names, types, ranges, and access flags are registered.

Typical fields:

```c
typedef struct {
    const char *name;
    param_type_t type;
    void *address;
    double minimum;
    double maximum;
    uint8_t writable;
    int (*before_write)(double value);
    void (*after_write)(void);
} weixin_param_t;
```

Use callbacks only when a parameter needs validation beyond min/max or must update derived state. Keep simple parameters callback-free.

Parameter names:

- Make names stable across firmware versions.
- Use groups such as `motor0.kp`, `control.target`, or `system.version`.
- Avoid spaces, localized display text, and volatile names generated from memory addresses.
- Keep production-only secrets, passwords, keys, and irreversible commands out of the parameter table.

## 5. List/get/set behavior

Implement behavior to match the active protocol:

- `list`: print one metadata row per parameter, then the completion marker with the count.
- `get`: return exactly one successful value line or one error line.
- `set`: validate existence, access, parsing, type, range, and project-specific safety, then write and read back the actual stored value.
- `help`: if supported, return a marker recognized by GUI protocol-check.

For integer writes, reject fractional values unless the active protocol explicitly defines rounding. Reject NaN, infinity, parse overflow/underflow, destination-type overflow, trailing characters, and extra command tokens before changing storage.

For floats, print enough precision for repeatable tuning. Six decimal places is usually adequate for the default GUI protocol, but the active protocol and device domain should decide.

## 6. Error handling

Use the active protocol's known error identifiers. Keep errors short and machine-readable.

Recommended categories when supported:

- missing or malformed argument
- unknown command
- parameter not found
- invalid type or parse failure
- out of range
- read-only parameter
- buffer or line too long
- safety interlock denied

Do not return a successful response if the firmware clamped, rounded, deferred, or rejected a write. Either read back the exact applied value or return an error that explains the denial.

## 7. Wave output

Wave output is telemetry, not parameter metadata. Keep it separate from the parameter table unless the firmware has a reason to expose the same variable both ways.

Implementation rules:

- Use the active plot prefix and separators.
- Put related channels on the same line when they should share a timestamp.
- Include units in names only if that is the project convention, for example `temp_c`.
- Rate-limit telemetry. Start slow enough for logs to remain readable, then increase only when measured.
- Avoid printing from fast control interrupts. Buffer values and print from a lower-priority context.

For a PID loop, useful channels might be target, feedback, error, output, and mode. For a power device, useful channels might be voltage, current, temperature, and fault code.

## 8. Persistence and runtime side effects

Treat persistence as a separate design decision. Do not automatically write flash on every `set`; it can wear flash and surprise users. If persistence is required, expose a guarded command or parameter only after the user explicitly asks for that behavior.

For live tuning:

- Validate that new values are safe for the current operating mode.
- Apply values atomically when used by interrupt/control code.
- Use critical sections or double-buffering when needed.
- Consider read-only status parameters for current mode, limits, and firmware version.

## 9. Porting notes

Bare-metal STM32:

- Use HAL/LL/SPL consistently with the existing project.
- Keep receive callbacks short.
- Prefer a ring buffer if UART traffic includes logs and commands together.

RTOS firmware:

- Use a UART RX task or queue.
- Protect parameter storage shared with control tasks.
- Avoid blocking lower-priority command handling behind high-rate telemetry.

C++ firmware:

- Keep the external protocol C-compatible if existing drivers are C.
- A small registry class is fine when it matches the project style.

Non-STM32 firmware:

- The protocol is UART text-based; it does not require STM32-specific APIs.
- Reuse the same table/line-parser concept with the platform's UART driver.
