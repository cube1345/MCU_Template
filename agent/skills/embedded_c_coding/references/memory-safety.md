# Memory and Concurrency Safety

- Prefer static or caller-owned storage when its lifetime is known.
- When dynamic allocation is necessary, allocate through the project allocator, check for failure, define one owner, and free exactly once.
- Bound all buffer accesses and validate lengths before indexing, copying, or converting values.
- Size RTOS task stacks from measured worst-case use; avoid large automatic objects on constrained stacks.
- Protect shared state with the project's mutex, critical-section, atomic, or queue mechanism. `volatile` alone is not synchronization.
- Define ISR-to-task communication explicitly and use APIs that are safe in the calling context.
- Prevent integer overflow, signed shifts, use-after-free, double-free, dangling pointers, and unchecked return values.
- Exercise creation, error, cancellation, and cleanup paths during review and testing.

For detailed Chinese checklists and examples, read [memory-safety_zh.md](memory-safety_zh.md).
