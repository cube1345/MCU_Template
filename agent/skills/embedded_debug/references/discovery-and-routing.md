# Discovery and Routing

Chinese version: [discovery-and-routing_zh.md](discovery-and-routing_zh.md).

## Discover the current environment

Before attaching to hardware, establish:

- Repository, board, MCU/core, and active firmware configuration.
- Matching ELF/AXF and map file, including timestamp and build identity.
- Probe family, transport, unique ID/serial, and whether multiple probes exist.
- Debug tool entry point and version.
- Supported target name and required device packs.
- SWD/JTAG frequency supported by the wiring and target.
- Reset methods and whether NRST is physically connected.
- Whether Keil, another debugger, or a programmer already owns the probe.

Read project-local `agent/debug/` documentation and scripts when present. A
project script is evidence about that project, not a portable default.

Do not automatically select a probe when multiple devices are present. Resolve
the intended UID from the project, current hardware, or the user.

## Route by task

| Task | Primary workflow |
| --- | --- |
| Compile/link or inspect build errors | Keil/build workflow |
| Flash/download firmware | Keil/programmer workflow with authorization |
| Read startup logs or protocol traffic | Serial workflow |
| Hang, fault, reset, CPU state | Embedded debug workflow |
| SPI/I2C/UART/PWM/reset timing | Signal capture workflow |
| Analog integrity, ringing, voltage, rise time | Oscilloscope/electrical workflow |

Use pyOCD only when the connected probe and selected target are supported. A
document mentioning ST-Link or J-Link does not prove that the local pyOCD setup
supports it. Route unsupported probes to verified vendor or project tooling.

## Stop and ask

Stop before a hardware-affecting action when:

- The target board, MCU, probe, or UID is ambiguous.
- The documented command conflicts with the current tool or hardware.
- The required target/device support is missing.
- Reset wiring is unknown for a requested hardware reset.
- Another tool may own the probe.
- The symbol file may not match the running firmware.
- The user may be preserving a fault scene or timing-sensitive experiment.

Ask only for the smallest missing fact needed to choose safely.
