# Lexer

## Purpose

The lexer converts raw source code into a stream of tokens with minimal semantic interpretation.

It uses a finite state machine driven by character categories.

## Character Classification

```cpp
enum class CharType {
    Invalid,
    Alpha,
    Num,
    Paren,
    Sym,
    Format,
    Quote,
};
```

Each input character is mapped to a `CharType` via `getCharType`, which determines lexer state transitions.

## Token Types

```cpp
enum class Type {
    Invalid,
    Keyword,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    Operator,
    ParenOpen,
    ParenClose,
};
```

Token classification is deferred until a token boundary is reached and depends on:

* Final FSM state
* Token contents
* Keyword/operator lookup tables

## Token Structure

```cpp
class Token {
public:
    std::string content;
    uint32_t line;
    Type type;
};
```

Each token stores:

* Its raw string representation
* Source line number
* Classified token type

## Token Stream

```cpp
class Stream {
    std::vector<Token> content;
    uint32_t index;
};
```

The Stream abstraction provides:

* peek() – Lookahead without consuming
* pop() – Consume next token
* expect() – Enforce grammar constraints
* has() – End-of-stream detection

This stream is consumed directly by the parser.

## Tokenization Process

```cpp
Lexer::Stream tokenize(std::string& source_path);
```

Key behaviors:

* Supports line comments
* Tracks line numbers
* Supports string literals
* Splits tokens based on FSM state transitions