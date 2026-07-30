# Post-Edit Safety Checklist

Run this checklist after every embedded C change.

1. Confirm the requirement, affected module boundaries, and hardware assumptions.
2. Review all changed code and its callers for ownership and lifecycle effects.
3. Check all array, pointer, length, and integer operations for bounds and overflow.
4. Check error propagation and cleanup on every failing path.
5. Check ISR, task, callback, and shared-resource concurrency rules.
6. Check blocking calls, timeouts, and run-to-completion behavior.
7. Check `const`, `static`, header guards, fixed-width types, braces, and naming.
8. Check that no HAL or register access violates the project layer boundary.
9. Build with the project warnings enabled and inspect all diagnostics.
10. Run relevant tests or perform a documented, non-destructive verification.
11. Review generated artifacts and confirm that no unintended binary or hardware action is included.
12. Re-run this checklist after every follow-up fix.

For the full Chinese 12-stage review procedure, read [safety-checklist_zh.md](safety-checklist_zh.md).
