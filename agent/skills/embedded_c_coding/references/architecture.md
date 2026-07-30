# Embedded C Architecture and OOP Patterns

Use explicit module boundaries and depend on abstractions, not concrete board or HAL details.

## Required design rules

- Give every module one responsibility and one reason to change.
- Expose a small public header and keep implementation structs private in the `.c` file.
- Use opaque types, such as `typedef struct UartDriver UartDriver_t;`, rather than exposing members.
- Use focused operations tables or callbacks for interchangeable behavior; document every callback's execution context.
- Notify state changes through registered callbacks or queues. Keep ISR and callback work short and non-blocking.
- Split oversized interfaces into lifecycle, I/O, and configuration interfaces.
- Make ownership, initialization order, shutdown, and error propagation explicit.

## Layering

Keep dependencies flowing downward only:

1. Application: business state machines; call driver and middleware APIs only.
2. Middleware: protocols, logging, and error services; call driver APIs.
3. Drivers: device behavior; call the platform abstraction only.
4. Platform/board: RTOS abstraction and board bindings; this is the boundary permitted to call HAL.
5. HAL: vendor implementation and registers.

Application code must not call `HAL_GetTick`, `HAL_GPIO_ReadPin`, or other HAL APIs directly. Use the project's platform or RTOS abstraction instead.

## Before adding a device

1. Find the existing platform-device registration and lifecycle pattern.
2. Define a narrow public interface and private state.
3. Implement non-blocking operations and callbacks for completion or errors.
4. Register the device at the documented initialization stage.
5. Check that all failure paths release resources in reverse initialization order.

For the detailed Chinese reference, read [architecture_zh.md](architecture_zh.md).
