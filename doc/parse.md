# GearLang compiler: Parser

The parser is the second stage of the GearLang compiler.

It consumes a `Lexer::Stream` of tokens and produces an AST (`Ast::Program`).

Parsing is **distributed** — each node type owns its own `parse` static method. There is no monolithic grammar file.

---

## Design overview

The parser is:

* **recursive descent**
* **distributed across node types**
* **recoverable** (continues after errors)
* **keyword-aware** (qualifier tracking via `keyword_list`)

---

## Entry point

```cpp id="i3l2f7"
Ast::Program Ast::Program::parse(Lexer::Stream& s)
```

Iterates the token stream until exhausted:

```cpp id="4qpnr3"
while(s.has()) {
    keyword_list.clear();
    program.content.push_back(NodeBase::parse(s));
}
```

### Notes

* `keyword_list` is cleared before each top-level node
* qualifiers like `export` are accumulated before dispatching

---

## Node dispatch

```cpp id="i3l2f7"
NodeBase::parse(Lexer::Stream& s)
```

This is the central dispatch point. It peeks at the current token and routes to the appropriate node parser.

---

### Qualifier handling

Before dispatching, the parser checks for qualifiers:

```cpp id="i3l2f7"
if(tok == "export") {
    keyword_list.push_back(s.pop()->content);
    return parse(s);
}
```

Qualifiers are:

* accumulated into `keyword_list`
* consumed and recurse back into `parse`
* checked later via `Ast::check_keyword(...)`

This means any node can query whether a qualifier was present:

```cpp
check_keyword("export")
```

---

### Dispatch table

Nodes are split into two groups based on whether they consume a trailing `;`:

#### No semicolon

```text
fn      → Function::parse
{       → Block::parse
while   → While::parse
if      → If::parse (+ optional Else)
```

#### Semicolon-terminated

```text
let     → Let::parse
do      → Do::parse
return  → Return::parse
extern  → ExternFn::parse
struct  → Struct::parse
include → Include::parse
<other> → Expr::parse
```

After parsing a semicolon-terminated node:

```cpp
s.expect(";", s.peek()->span);
```

---

### EOF handling

If the stream is empty at dispatch time:

```cpp
Error::throw_error_and_recover(
    Span{},
    "Unexpected EOF",
    Error::ErrorCodes::UNEXPECTED_EOF, s
);
```

---

## Expression parsing

Expressions are parsed via a two-level recursive descent:

```cpp id="3ycb2y"
Expr::parse      → Expr::parseExpr
                       → Expr::parseTerm
```

---

### `parseExpr` — binary operations

Parses one left-hand term, then checks for an operator:

```cpp
pExpr left = parseTerm(s);

if (s.peek()->type != Lexer::Type::Operator)
    return left;
```

If an operator follows, it is consumed and a right-hand term is parsed:

```cpp id="6d0yzf"
return ExprOp(type, left, right, span)
```

#### Supported operators

```text id="9b1c8r"
+  -  *  /  <  >  ==  !=  >=  <=
```

Single-character operators are matched by first character. Multi-character operators (`==`, `!=`, `>=`, `<=`) are matched via `match_op`.

---

### `parseTerm` — atoms

Handles the building blocks of expressions:

```text
( expr )         → grouped expression
FloatLiteral     → ExprLitFloat
IntegerLiteral   → ExprLitInt
StringLiteral    → ExprLitString
CharLiteral      → ExprLitChar
@                → ExprDeref
#                → ExprAddress
Identifier (     → ExprCall
Identifier =     → ExprAssign
Identifier       → ExprVar
```

Unrecognized tokens trigger a recoverable error:

```cpp
Error::throw_error_and_recover(
    span,
    "Unexpected token.",
    Error::ErrorCodes::UNEXPECTED_TOKEN, s
);
```

---

## Statement parsing

---

### `Let`

```gr id="q3uxbq"
let x = 10;
let x: i32 = 10;
let x: i32;
```

Sequence:

1. Expect `let`
2. Read identifier (target name)
3. If `:` follows → parse type annotation
4. If no type, or `=` follows → expect `=` and parse expression
5. Check `export` qualifier

```cpp id="0h2y2q"
Let {
    target, expr, ty,
    is_global, is_public
}
```

Type annotation and initializer are both optional independently — but at least one must be present.

---

### `Return`

```gr
return expr;
```

Sequence:

1. Expect `return`
2. Parse expression

---

### `Block`

```gr id="q3uxbq"
{
    ...
}
```

Parses nodes until the matching `}` is found.

Tracks nesting depth to handle nested blocks:

```cpp id="xq7p6k"
if(t->type == BraceOpen)  brace_count++;
if(t->type == BraceClose) brace_count--;
if(brace_count == 0) break;
```

Each inner node is parsed via `NodeBase::parse`.

---

### `If` / `Else`

```gr id="q3uxbq"
if cond { ... }
if cond { ... } else { ... }
```

`If::parse`:

1. Expect `if`
2. Parse condition expression
3. Parse body node

After `If` is parsed, the caller checks for `else`:

```cpp
if(s.peek()->content == "else")
    return Else::parse(std::move(if_expr), s);
```

`Else::parse` takes ownership of the already-parsed `If`, then parses the else body.

---

### `While`

```gr
while cond { ... }
```

Sequence:

1. Expect `while`
2. Parse condition expression
3. Parse body node

---

### `Do`

```gr
do { ... } while cond;
```

Sequence:

1. Expect `do`
2. Parse body node
3. Expect `while`
4. Parse condition expression

Note the condition comes **after** the block, unlike `While`.

---

### `Struct`

```gr
struct Name { ... }
```

Sequence:

1. Read name via `s.next()`
2. Parse type via `Sem::Type(s)`

Codegen is triggered immediately during analysis.

---

### `Include`

```gr
include("C", "header", "stdio.h");
```

Sequence:

1. Expect `include`
2. Expect `(`
3. Read: `lang`, `type`, `file` as string literals separated by commas
4. Expect `)`

---

## Function parsing

Functions have a shared header parsing path used by both `Function` and `ExternFn`.

---

### Header parsing

```cpp id="f94nl2"
parse_function_header(s, span, requires_names, is_variadic)
```

Returns:

```cpp
{ Sem::Type ty, string name, deque<Argument> args }
```

Sequence:

1. Read function name
2. Delegate to `parse_function_args`
3. Check for `returns` keyword → parse return type
4. Default return type is `void`

---

### Argument parsing

```cpp
parse_function_args(s, span, requires_names, is_variadic, ty)
```

Only runs if `(` is present — functions with no args omit parens entirely.

Argument loop:

* Checks for `...` → marks variadic, breaks
* Otherwise parses `Argument` nodes
* Allows trailing comma (checks for `)` after each `,`)

---

### `Function`

```gr
fn name(args) returns Type { ... }
fn name { ... }
```

Sequence:

1. Expect `fn`
2. Parse header (name, args, return type)
3. Parse body block
4. Check `export` qualifier → sets `is_public`

---

### `ExternFn`

```gr
extern fn name(args) returns Type;
extern("C") fn name(args) returns Type;
```

Sequence:

1. Expect `extern`
2. Optionally parse ABI string: `("C")` → sets `no_mangle = true`
3. Expect `fn`
4. Parse header

If an unknown ABI string is given:

```cpp
throw std::runtime_error(tok->content);
```

---

## Variable parsing

---

### `ExprVar`

Dispatches to `ExprStructParam` if the identifier contains `.`:

```cpp id="9v4dfg"
if(token.content.contains('.'))
    return ExprStructParam::parse(token, s);
```

---

### `ExprStructParam`

```gr id="92cv9p"
s.x
```

Splits the identifier on `.` to extract struct name and field name.

---

### `ExprAssign`

```gr
x = expr
```

Sequence:

1. Receives already-parsed `ExprVar`
2. Expect `=`
3. Parse expression

---

### `ExprAddress` / `ExprDeref`

```gr id="p6vxg7"
#x    // address-of
@x    // dereference
```

Both:

1. Consume the prefix token (`#` or `@`)
2. Pop the next token
3. Parse it as `ExprVar`

---

## Literal parsing

All literals follow the same pattern: pop one token, convert content, return node.

| Node | Token type | Conversion |
|---|---|---|
| `ExprLitInt` | `IntegerLiteral` | `stoi` → `uint64_t` |
| `ExprLitFloat` | `FloatLiteral` | `stod` → `double` |
| `ExprLitString` | `StringLiteral` | raw string content |
| `ExprLitChar` | `CharLiteral` | first character of content |

`ExprLitString::to_string()` re-escapes `\n` and `\t` for debug output.

---

## `to_string` output

Every node implements `to_string()` which produces JSON-like debug output:

```text
{ "type":"ExprOp", "oper":"Add", "left":..., "right":... }
```

`Program::to_string()` produces a JSON array of all top-level nodes.

---

## Invariants

The parser guarantees:

* Every node has a valid `Span` derived from its tokens
* Semicolons are consumed by the dispatcher, not individual node parsers
* Qualifiers are cleared before each top-level node
* `Block` correctly handles arbitrary nesting depth
* Function argument lists allow trailing commas

---

## Design notes

* Parsing is fully **distributed** — no central grammar table
* `keyword_list` is a **global mutable vector**, cleared per statement
* `Else` takes ownership of a pre-parsed `If` rather than re-parsing the condition
* `ExternFn` ABI handling uses a hard `throw` for unknown ABIs, not the error system
* `parseTerm` is the only place where unknown tokens produce recoverable errors

---

## Known limitations

* Binary expressions are **flat** — no precedence levels beyond left-associative single operator
* No unary operators (negation, logical not)
* `requires_names` parameter in function header parsing is threaded through but unused
* Unknown ABI in `extern` throws a C++ exception rather than a compiler error

---

## Future work

* Unary operator support
* Remove `requires_names` or implement it
* Consistent error handling in `ExternFn` ABI parsing
* Multi-return / tuple support