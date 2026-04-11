# GearLang compiler: Lexer

The lexer is the first stage of the GearLang compiler.

It converts raw source text into a structured stream of tokens. These tokens are then consumed by the parser.

This stage is still intentionally “dumb”, but it does more than a trivial tokenizer:

* Stateful scanning
* Inline classification
* String + char handling
* Comment stripping
* Source tracking

---

## Responsibilities

The lexer is responsible for:

* Splitting source into tokens
* Classifying tokens into concrete types
* Tracking full source spans (file, line, column, indices)
* Handling:

  * strings
  * char literals
  * escape sequences
  * line + block comments

The lexer does **not**:

* Validate syntax correctness
* Resolve identifiers
* Perform type checking

---

## Input / Output

### Input

Either:

* a file (`tokenize(path)`)
* or a raw string (`tokenize_by_string`)

### Output

A `Lexer::Stream`:

```cpp
Stream {
  std::vector<Token> content;
}
```

Each `Token`:

```cpp
Token {
  std::string content;
  Span span;
  Type type;
}
```

---

## Token metadata (Span)

Every token carries a full `Span`:

```cpp
Span {
  file;
  line;
  col;
  start;
  end;
}
```

This is used for:

* error reporting
* diagnostics
* later compiler stages

The lexer guarantees all tokens have valid span data.

---

## Token classification model

Token classification is split into two parts:

1. **Character-level classification (`CharType`)**
2. **Token-level classification (`Type`)**

---

### CharType (low-level state)

Each character is mapped into a `CharType`:

```cpp
enum class CharType {
  Alpha, Num, Paren, Brace,
  Sym, Format, Quote, Apost,
  Caret, Semi, Hash, At,
  Invalid
};
```

This is used to:

* drive token boundaries
* guide classification

Notably:

* `.` is treated as `Num` (for floats / ellipsis)
* `_` is treated as `Alpha`
* whitespace is `Format`

---

### Token Type (final classification)

After a token is formed, it is classified into:

```cpp
Type {
  Keyword, Identifier,
  IntegerLiteral, FloatLiteral,
  StringLiteral, CharLiteral,
  Operator,

  ParenOpen, ParenClose,
  BraceOpen, BraceClose,

  Equal, Comma, Semi,
  Caret, Hash, At,
  Ellipsis
}
```

---

## Keyword and operator tables

The lexer does not hardcode keywords or operators.

Instead, they are loaded at runtime:

```cpp
data/tokens.txt
```

* First line → keywords
* Second line → operators

These are stored in:

```cpp
Table {
  std::unordered_set<std::string> keywords;
  std::unordered_set<std::string> operators;
}
```

This makes the lexer:

* configurable
* decoupled from language evolution

---

## Core lexing model

The lexer operates as a **single forward pass** over the source.

It maintains:

* `index` → absolute position
* `line`, `col` → location tracking
* `state_old` → previous character type
* `tok` → current token buffer

---

## Token boundary detection

Token boundaries are determined by:

```cpp
state_new != state_old
|| is_single_char_token(state_new)
|| is_single_char_token(state_old)
```

### Exceptions

Alpha ↔ Num transitions are allowed:

```cpp
abc123
123abc
```

These do NOT create boundaries.

---

## Flush mechanism

Tokens are finalized via a `flush()` lambda.

Flush occurs when:

* a boundary is detected
* a token completes (string, char, etc.)

### Flush rules

A token is ignored if:

* empty (unless string)
* only whitespace
* inside a comment

Otherwise:

```cpp
tok.type = classify(...)
out.content.push_back(tok)
```

---

## String handling

Strings are handled inline with state flags:

```cpp
bool is_string;
bool token_is_string;
```

### Behavior

* `"` toggles string mode
* content is accumulated raw
* escapes are processed inline

### Escape handling

```cpp
\n  \t  \\  \0
```

Unknown escapes:

* emit warning
* pass through

### Errors

Unterminated string:

```gr
"hello
```

Triggers:

* immediate error on newline or EOF

---

## Char literals

Char literals are handled as a special-case fast path:

```gr
'a'
'\n'
```

Behavior:

* reads exactly one character (or escape)
* enforces closing `'`

Invalid form:

```gr
'ab'
```

→ error

---

## Comment handling

Two types:

### Line comments

```gr
// comment
```

* Enabled when `/` followed by `/`
* Ends at newline

---

### Block comments

```gr
/* comment */
```

* Enabled when `/` followed by `*`
* Ends at `*/`

### Notes

* Comments are completely discarded
* They do not produce tokens
* They are ignored unless inside strings

---

## Special tokens

### Ellipsis

```gr
...
```

Handled as:

```cpp
Type::Ellipsis
```

Detected during classification.

---

### Single-character tokens

Defined via:

```cpp
is_single_char_token(CharType)
```

Includes:

* parens
* braces
* `^ ; # @`

These always form standalone tokens.

---

## Number handling

Numbers are classified based on content:

### Integer

```gr
123
```

→ `IntegerLiteral`

---

### Float

```gr
3.14
```

→ `FloatLiteral`

Rules:

* must contain exactly one `.`
* multiple `.` → error

---

### Mixed alpha

```gr
123abc
```

→ treated as `Identifier`

---

## Stream API

The lexer outputs a `Stream`, which is consumed by the parser.

### Navigation

```cpp
peek()  // current
next()  // lookahead
pop()   // consume
back()  // rewind 1
```

---

### Expectation helpers

```cpp
expect("let")
expect(Type::Identifier)
```

On failure:

* emits parser-facing error
* attempts recovery

---

### Debugging

```cpp
dump()
to_string()
```

`to_string()` format:

```text
["content", line, "Type"]
```

---

## Error handling

The lexer integrates with the error system:

```cpp
Error::throw_error(...)
Error::throw_warning(...)
```

### Error cases

* Invalid number (`1.2.3`)
* Unexpected symbol
* Unterminated string
* Invalid char literal

### Behavior

* Most errors are **fatal**
* Some (like escapes) are warnings

---

## Invariants

The lexer guarantees:

* Every emitted token is classified
* Every token has a valid span
* No comments appear in output
* Strings and chars are fully resolved

---

## Design notes

* Classification is deferred until flush (not per-character)
* State machine is implicit, not table-driven
* Token buffer is reused (minimizes allocations)
* Lexer is tightly coupled to error system

---

## Known limitations

* No unicode support
* No nested block comments
* No preprocessing stage
* Limited numeric formats

---

## Future work

* Unicode / UTF-8 support
* Better recovery after errors
* Configurable literal formats
* Performance tuning (arena allocators, etc.)
