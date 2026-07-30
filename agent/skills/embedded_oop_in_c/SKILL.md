---
name: oop-in-c
description: "Design, implement, refactor, and review production-grade MCU firmware in C using selective OOP-in-C patterns: base structs, const ops tables, explicit lifecycle and ownership, container_of downcasts, board-level binding, and HAL-aware concrete drivers. Use for embedded firmware, STM32/CubeMX GPIO or PWM drivers, HAL/platform layers, reusable device frameworks, or C modules with multiple interchangeable implementations. Do not trigger for teaching OOP basics, translating C++ syntax mechanically, host-only demos, or forcing polymorphism into simple single-implementation modules."
---

# Engineering OOP in C

Build embedded C systems with explicit object layout, controlled dependencies,
MCU hardware boundaries, and the smallest useful abstraction. Treat OOP
as an engineering tool, not a project-wide requirement.

## Workflow

1. Inspect repository rules, compiler/linker constraints, RTOS model, memory
   policy, error model, initialization order, and nearby module conventions.
2. Keep a plain prefixed C module when there is one stable implementation and
   no useful substitution boundary.
3. Add `base + const ops` only for interchangeable implementations. Add board
   binding to hide hardware choice; add a platform interface to cross MCU/SDK
   boundaries; use registration/init sections only when they reduce coupling.
4. Define ownership, lifecycle, valid states, required/optional operations,
   callback context, blocking behavior, thread/ISR safety, and error semantics
   before writing a backend.
5. Implement one vertical path: interface -> concrete MCU backend -> board
   binding -> target build and board-level smoke test.
6. Read `references/review-checklist.md` before finishing.

## Engineering Rules

- Preserve project conventions when they conflict with this generic workflow.
- Prefer static or caller-owned storage. Do not introduce heap allocation unless
  project policy permits it and ownership/exhaustion behavior is explicit.
- Keep public headers minimal; make helpers, concrete objects, and ops tables
  file-private with `static`.
- Define immutable ops tables as `static const` with designated initializers.
- Bind an ops table during initialization. Do not mutate it at runtime without
  an explicit synchronized strategy-replacement requirement.
- Embed the base as the first child member by default. Upcast with
  `&child.base`; downcast with the project's `container_of` implementation.
- Expose only base/interface handles to application code. Keep concrete types,
  pin/channel/address data, and vendor SDK types in board or arch backend code.
- Keep HAL/register access in a concrete driver or platform backend; never leak
  HAL types into the base or application API.
- Validate external inputs at boundaries and assert internal invariants in debug
  builds. Preserve a release-build error path for recoverable failures.
- Validate required ops at initialization. For optional ops, return a documented
  unsupported error or implement a documented default; never fake success.
- Propagate meaningful project error codes. Do not silently discard failures.
- Document ownership, callback context, locking, blocking, and ISR restrictions
  for every public API. `volatile` alone is not synchronization.
- Avoid deep inheritance and hidden mutable globals. Prefer composition and
  per-device ops objects when multiple instances can coexist.
- Do not add educational logging or tutorial comments to production code.

## Required References

- Read `references/engineering-patterns.md` before designing/refactoring a
  polymorphic module, driver family, platform layer, or registration system.
- Read `references/module-template.md` before creating a base/ops/concrete
  module family.
- Read `references/mcu-led-example.md` and use
  `assets/led-mcu-example/` when a complete STM32 CubeMX/HAL GPIO/PWM/board
  reference is useful.
- Read `references/review-checklist.md` before completing implementation or
  review work with this skill.

## Output Expectations

When implementing, explain why OOP is warranted, identify the abstraction
boundary, group changes by interface/backend/binding/tests, and state what was
verified. When reviewing, prioritize lifetime, layout, dispatch, errors,
concurrency, and hardware-boundary defects; flag needless abstraction.
