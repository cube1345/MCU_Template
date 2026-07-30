# Hardware Interaction Rules

Treat every peripheral operation as potentially asynchronous, stateful, and fallible.

- Use the project platform or driver API; do not bypass its layering with direct register or HAL access.
- Verify the actual register, clock, pin, DMA, interrupt, and peripheral state before assuming an API is non-blocking or complete.
- Keep ISRs short. Defer processing to a task, queue, or state machine.
- Do not busy-wait in event-driven or run-to-completion paths.
- Check every HAL and driver return value, time out bounded waits, and expose errors to the caller or callback.
- Define initialization and deinitialization order, including clocks, GPIO, DMA, IRQ, and shared resources.
- Protect shared hardware state with the project's documented synchronization primitive.
- Never change live hardware, flash, option bytes, or safety-critical registers without explicit authorization.

For the complete Chinese hardware guide, read [hardware-interaction_zh.md](hardware-interaction_zh.md).
