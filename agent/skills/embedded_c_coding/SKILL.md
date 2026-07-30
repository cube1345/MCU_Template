---
name: embedded-c-coding
description: Embedded C language coding specification, suitable for safety-critical embedded systems. Enforce OOP design patterns, memory safety, thread safety, hardware interaction rules and mandatory post-edit security reviews. Use this skill when writing, modifying, or reviewing C code, embedded firmware, drivers, HAL code, RTOS tasks, or any code that runs on an embedded platform. It will also be triggered when the user mentions C language, embedded system, MCU, STM32, driver, peripheral or firmware development.
---

# Embedded C Coding Standards for Safety-Critical Systems

Chinese version: [SKILL_zh.md](SKILL_zh.md).

This is safety critical embedded system code. Defects can lead to system failures or even safety incidents.
Every line of code must be written and reviewed with the utmost care.

## Core Principles

1. **Safety first** — Treat every change to this safety-critical software as consequential.
2. **SOLID in C** — Use function-pointer interfaces, opaque types, and dependency injection.
3. **Clean Code** - The function only does one thing, the name reveals the intention, and there is no repetition.
   Side-effect-free, consistent abstraction level
4. **OOP in C** - Use object-oriented design patterns even in C;
   Access the device only through a unified external interface
5. **Zero Tolerance Blocking** - If the application layer uses event-driven framework/RTC (run to completion)
   Execution model, the entire call chain must be non-blocking
6. **Verify to the bottom layer** - Check the hardware interaction to the register level;
   Never assume an API is non-blocking
7. **Defensive Programming** - Assert internal contracts, validate external input, propagate errors,
   Never swallow failure silently

## Decision tree: which reference document should be read

Before writing or modifying code, determine which reference documentation applies:

```
编写/修改代码？
├── 设计模块结构、接口、回调？
│   ├── 阅读 references/architecture.md
│   └── 阅读 references/design-patterns.md
│
├── 编写函数、命名、错误处理？
│   └── 阅读 references/clean-code.md
│
├── 使用堆内存、线程、共享资源？
│   └── 阅读 references/memory-safety.md
│
├── 与硬件、外设、HAL交互？
│   └── 阅读 references/hardware-interaction.md
│
├── 任何代码修改？
│   ├── 阅读 references/code-style.md
│   └── 阅读 references/safety-checklist.md（强制）
│
└── 编辑完成？
    └── 阅读 references/safety-checklist.md
        并执行每一个检查项（阶段1-12）
```

**When in doubt, read all reference documentation. ** For security critical code,
It’s better to over-check than under-check.

## Mandatory post-edit agreement

Every time the code is modified, no matter how small the modification is, it must be executed
Full safety review in `references/safety-checklist.md`. This is not optional.
A single line of code change can introduce a memory leak, race condition, or stack overflow.
Also re-run the checklist after each subsequent fix - fixing one problem may introduce another.

## Cheat Sheet: Hard Rules

The following are non-negotiable constraints:

| Rules | Restrictions |
|------|------|
| Maximum function length | 80 lines |
| Maximum nesting depth | 4 levels |
| Maximum row width | 80 columns |
| Maximum number of function parameters | 5 (if exceeded, they will be combined into a structure) |
| Large local variables in threads | Evaluate stack usage, use heap when critical |
| Magic numbers | Forbidden, use macros |
| Dead code/unused code | Must be removed |
| Non-public symbols | must be declared `static` |
| Unmodified pointer | Must use `const` |
| Header file | There must be a header file to protect the macro |
| Comment language | English is recommended, or consistent with the project |
| Author in comments | weixin |

## Context-aware encoding

Before writing any code, check the surrounding code base:

1. **Signal/Slot Usage** - Check how sibling files in the same directory use signals,
   Callback and event mechanisms; follow the same pattern
2. **Header file inclusion path** - Search how other files in the project include the same header file;
   Use the same path format
3. **Heap Memory API** - Embedded platforms may use non-standard malloc/free;
   Check how other files allocate and free memory
4. **Device Open/Close** - Do not access device pointer directly; find other modules
   How to turn on/off the same device and follow the pattern
5. **Coding Style** - Match the structure and format conventions of files in the same directory

## Driver Development Checklist

When writing a new driver:

1. Internal working thread + message queue to implement non-blocking operations
2. Periodic polling remains inside the driver and is not exposed to the outside world.
3. Error status changes are notified to the application layer through callbacks
4. Do not expose interfaces unrelated to business
5. Document the instructions for using the application layer after completion, including: initialization sequence,
   Thread safety guarantees, callback context and cleanup requirements

## SOLID quick check

When designing or modifying a module structure:

- **SRP (Single Responsibility)**: One module = one responsibility = one reason for change
- **OCP (Opening and Closing Principle)**: Extension through function pointer table instead of modifying the code
- **LSP (Liskov Substitution)**: All interface implementations adhere to the same contract
- **ISP (Interface Isolation)**: Users only rely on the interface they use
- **DIP (Dependency Inversion)**: High-level dependencies on abstractions, not low-level ones

## Clean Code quick check

When writing any function:

- **One thing**: The function does only one thing
- **Same level**: All operations are at the same abstraction level
- **No side effects**: The query function does not modify the state
- **DRY**: If copying and pasting, extract shared functions
- **Table driver**: Use lookup tables to replace lengthy if-else/switch
- **Defensive**: Assert contracts, validate input, propagate errors

## Reference documentation

Detailed specifications organized by field:

| Reference Documentation | When to Read |
|---------|---------|
| [architecture.md](references/architecture.md) | Module design, OOP mode, interface, callback |
| [design-patterns.md](references/design-patterns.md) | SOLID principles, C language GoF pattern (state machine, observer, strategy, factory) |
| [clean-code.md](references/clean-code.md) | Clean Code + Huawei specifications: naming, defensive programming, error handling, const/static |
| [memory-safety.md](references/memory-safety.md) | Heap allocation, thread safety, shared resources, defensive coding |
| [hardware-interaction.md](references/hardware-interaction.md) | HAL, registers, peripherals, event-driven framework / RTC |
| [code-style.md](references/code-style.md) | Format, naming, comments, macros, header file protection |
| [safety-checklist.md](references/safety-checklist.md) | **Enforced after every edit** - 12-stage safety review |
