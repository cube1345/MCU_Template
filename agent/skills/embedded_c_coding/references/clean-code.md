# Clean Code for Embedded C

Write code that states its intent, has bounded behavior, and reports failure explicitly.

## Rules

- Make each function do one thing and keep operations at one abstraction level.
- Prefer names that reveal purpose: `retry_count`, `UartDriver_SendAsync`, and `is_connected`.
- Limit functions to five parameters; group related values in a configuration or request struct.
- Separate queries from commands. A query must not change state or hardware.
- Validate all untrusted input at runtime; use assertions only for internal programmer contracts.
- Define project-wide error codes and either handle or return every error.
- Prefer `const` for read-only pointed-to data and `static` for all non-public functions and file-private state.
- Avoid globals. If one is unavoidable, document ownership and concurrency protection.
- Replace repeated condition chains with tables or small helper functions when that improves clarity.

For complete examples and rationale, read [clean-code_zh.md](clean-code_zh.md).
