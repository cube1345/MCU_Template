# IMU_9

STM32U535CCT6 bare-metal empty firmware profile.

## Build

The project uses the GNU Arm toolchain and Ninja. From the project root:

```powershell
cmake --preset Debug
cmake --build build\Debug
```

The build produces:

- `build/Debug/IMU_9.elf`
- `build/Debug/IMU_9.hex`
- `build/Debug/IMU_9.bin`
- `build/Debug/IMU_9.map`

## Current profile

The firmware initializes HAL, the 160 MHz system clock, and the CubeMX GPIO
startup code, then remains in the empty main loop. The current CubeMX profile
contains the HSE clock and SWD pins only.

The reusable `0_hal` through `6_apps` source tree is kept in the repository for
later migration, but is intentionally excluded from this empty build. Those
modules still contain STM32F4 include names and require STM32U5 peripheral and
board bindings before they can be enabled.
