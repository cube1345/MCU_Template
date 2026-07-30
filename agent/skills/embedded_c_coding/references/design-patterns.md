# Embedded C Design Patterns

Use patterns only where they reduce coupling or make hardware behavior testable.

## SOLID in C

- **Single responsibility:** one module, one responsibility.
- **Open/closed:** extend behavior through operations tables or callbacks.
- **Liskov substitution:** all implementations honour the same contract.
- **Interface segregation:** expose small, client-specific interfaces.
- **Dependency inversion:** inject platform, transport, and hardware dependencies through abstractions.

## Useful patterns

- **State machine:** model legal states, transitions, and entry/exit actions explicitly.
- **Observer/callback:** report data-ready, completion, and error events without polling globals.
- **Strategy/operations table:** select alternate hardware or protocol implementations behind one interface.
- **Factory/constructor:** validate configuration and establish ownership in one creation path.
- **Adapter:** isolate vendor HAL, board support, or third-party APIs behind a project interface.

Document lifetime, ownership, concurrency, and error behavior for every pattern. Do not introduce polymorphism into a simple single-implementation module.

For detailed Chinese patterns and examples, read [design-patterns_zh.md](design-patterns_zh.md).
