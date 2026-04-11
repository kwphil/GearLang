# GearLang compiler: Error Handling

The error system in GearLang is centralized and span-based.

All compiler stages report errors through the same interface:

```cpp
Error::throw_error(...)
Error::throw_error_and_recover(...)
Error::throw_warning(...)
```

Errors are collected, printed with source context, and may either:

* terminate immediately
* or allow the compiler to continue (recovery mode)

---

## Design overview

The error system is built around three ideas:

1. **Span-based reporting** (exact source location)
2. **Centralized error tracking**
3. **Recoverable vs fatal errors**

---

## Error codes

All errors are categorized using:

```cpp
enum class ErrorCodes {
  OK,
  EXPECT_VALUE,
  UNEXPECTED_TOKEN,
  UNEXPECTED_EOF,
  UNKNOWN_TYPE,
  VARIABLE_ALREADY_DEFINED,
  VARIABLE_NOT_DEFINED,
  FUNCTION_NOT_DEFINED,
  FUNCTION_INVALID_ARGS,
  QUALIFIER_NOT_ALLOWED,
  INVALID_AST,
  UNKNOWN_FILE,
  BAD_TYPE,
};
```

---

### Purpose

Error codes are used to:

* categorize failures
* report summary at the end
* provide exit codes

---

## Core components

---

### Global error state

```cpp
std::unordered_set<ErrorCodes> codes;
```

Tracks all errors encountered during compilation.

---

### Source file tracking

```cpp
std::vector<std::string> error_split_file;
```

Stores the entire source file line-by-line.

Used for:

* printing context
* highlighting errors

---

## Error output format

All errors follow this structure:

```text
Error: <message>
<line>:<col>: <source line>
          ^^^^^
```

### Example

```text
Error: Unexpected token
3:5: let x = ;
         ^
```

---

### Highlighting

The underline is calculated using:

```cpp
span.start → span.end
```

This ensures:

* multi-character tokens are fully highlighted
* precise error visualization

---

## Error types

---

### Fatal errors

```cpp
[[noreturn]] throw_error(...)
```

### Behavior

* prints error immediately
* inserts error code
* calls `flush()`
* exits program

```cpp
exit((int)code);
```

---

### When used

* lexer failures (invalid tokens)
* unrecoverable parser states
* critical semantic failures

---

## Recoverable errors (parser)

```cpp
throw_error_and_recover(span, msg, code, Stream& s)
```

### Behavior

1. Print error
2. Store error code
3. Advance token stream to a **sync point**

---

### Sync strategy

```cpp
while(s.has()) {
  auto curr = s.pop()->type;
  if(curr == Semi || curr == BraceClose) break;
}
```

The parser skips tokens until:

* `;` (statement boundary)
* `}` (block boundary)

---

### Why this works

These tokens represent:

* natural statement termination points
* safe places to resume parsing

---

### Example

```gr
let x = ;
let y = 10;
```

Instead of stopping at the first error, the parser:

* skips to `;`
* continues parsing `let y = 10;`

---

## Recoverable errors (analyzer)

```cpp
throw_error_and_recover(span, msg, code)
```

### Behavior

* prints error
* stores code
* **does not attempt structural recovery**

---

### Why no recovery?

At the analysis stage:

* structure is already fixed (AST exists)
* no token stream to resync
* recovery means “continue analyzing other nodes”

---

## Warnings

```cpp
throw_warning(span, msg)
```

### Behavior

* prints warning
* does NOT store error code
* does NOT stop execution

---

### Example

```cpp
Unknown escape char: \x
```

Warnings are:

* informational
* non-fatal

---

## Flush mechanism

```cpp
Error::flush()
```

### Behavior

* checks if any errors occurred
* prints all error codes
* exits program

---

### Output format

```text
Codes thrown: E001, E004, E010
```

---

### Exit behavior

```cpp
exit(code);
```

* uses last error code encountered

---

## Setup

Before compilation begins:

```cpp
Error::setup_error_manager(filename, disable_color)
```

### Responsibilities

* loads source file into memory
* enables/disables ANSI coloring

---

## ANSI formatting

The system uses ANSI escape codes for styling:

* red → errors
* yellow → warnings
* magenta → span info

Can be disabled via:

```cpp
disable_color = true
```

---

## Integration with compiler stages

---

### Lexer

Uses:

* `throw_error` (fatal)
* `throw_warning`

Lexer errors are typically unrecoverable:

* invalid characters
* malformed literals

---

### Parser

Uses:

* `throw_error_and_recover(stream)`

This allows:

* multiple syntax errors per run
* partial AST construction

---

### Analyzer

Uses:

* `throw_error_and_recover(no stream)`

This allows:

* multiple semantic errors
* full-program analysis even with failures

---

### Codegen

Generally assumes:

* no errors exist

If reached with errors:

* behavior is undefined or incomplete

---

## Invariants

The error system guarantees:

* All errors include span data
* Errors are printed immediately when encountered
* Recoverable errors do not terminate compilation
* Fatal errors always terminate

---

## Design notes

* Error handling is **side-effect driven** (global state)
* No exception-based flow control
* Recovery is **stage-specific**
* Parser recovery is intentionally simple (not grammar-aware)

---

## Limitations

* No error deduplication
* No severity levels beyond error/warning
* Recovery is heuristic (not precise)
* Exit code is last error (not most severe)

---

## Future work

* Structured diagnostics (JSON output, LSP)
* Better recovery strategies (parser-aware sync)
* Error ranges with multi-line support
* Severity levels (info, warning, error, fatal)

---

# Why this matters (for the parser doc)

This system explains:

* why `Stream::expect` doesn’t crash immediately
* how parsing continues after failure
* why AST can exist in a partially invalid state
