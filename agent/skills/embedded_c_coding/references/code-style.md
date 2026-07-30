# Embedded C Code Style

Follow the project's established style first. Apply these mandatory limits unless the project documents a stricter rule.

| Item | Rule |
| --- | --- |
| Line width | At most 80 columns |
| Function length | At most 80 lines |
| Nesting | At most four levels; use guard clauses |
| Numeric literals | Name non-trivial values with macros |
| Control statements | Always use braces |
| Integer types | Use fixed-width `stdint.h` types |

Use uppercase snake case for macros, `PascalCase_t` for types, `Module_Action` for public functions, and lower snake case for locals and struct members. Prefix file-static state with `s_`; reserve `g_` for documented global state.

Every public function needs a documentation comment covering parameters, ownership, return values, error cases, callback context, and thread safety. Delete dead or commented-out code; version control retains history.

Headers must have guards, include the types they require, and follow the project's include-path and include-order conventions.

For the complete Chinese style guide, read [code-style_zh.md](code-style_zh.md).
