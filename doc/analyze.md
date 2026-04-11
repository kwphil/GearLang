# GearLang compiler: Analyzer

The analyzer is the semantic analysis stage of the GearLang compiler.

It operates on the AST after parsing, before code generation.

Its responsibilities are:

* resolving variable and function references
* assigning types to all expressions
* validating type compatibility
* tracking scope and control flow
* marking unreachable nodes as dead

---

## Design overview

The analyzer is:

* **pass-based** (walks the full AST once)
* **scope-stacked** (nested scopes via a vector of maps)
* **distributed** (each node implements its own `analyze`)
* **non-transforming** (annotates nodes in place, does not rebuild the tree)
* **optionally traceable** (structured diagnostic logging via `Logger`)

---

## Core class

```cpp
class Sem::Analyzer {
    vector<shared_ptr<Scope>> active_scopes;
    unordered_map<string, Func> function_list;
    Logger logger;
    bool dump_self;
}
```

---

## Semantic value types

Three structs carry semantic information between analysis steps:

### `ExprValue`

Returned by `Expr::analyze`:

```cpp
struct ExprValue {
    bool is_const;
    Type ty;
}
```

Used to propagate type and constness upward during expression analysis.

---

### `Variable`

Stored in scopes:

```cpp
struct Variable {
    string name;
    Type type;
    uint8_t is_global;  // 0=false, 1=true, 2=needs check
    NodeBase* let_stmt;
}
```

`let_stmt` links a variable use back to its declaration node. This is how `ExprVar::let` gets populated.

---

### `Func`

Stored in the function registry:

```cpp
struct Func {
    string name;
    Type ret;
    vector<Type> args;
    bool is_variadic;
}
```

---

## Entry point

```cpp
void Analyzer::analyze(deque<unique_ptr<NodeBase>>& nodes)
```

Iterates the program's top-level nodes. For each node, dispatches via:

```cpp
constexpr bool analyze_nodebase(unique_ptr<NodeBase>* node, Analyzer& analyzer)
```

This helper downcasts to either `Stmt` or `Expr` and calls the appropriate `analyze` method. Statements return `bool` (control flow); expressions return `unique_ptr<ExprValue>`.

---

## Scope management

Scopes are stored as a stack of shared maps:

```cpp
vector<shared_ptr<Scope>> active_scopes;
```

### Operations

```cpp
weak_ptr<Scope> new_scope(Span span)   // push
void delete_scope(Span span)           // pop
```

Variable lookup walks the stack from top (innermost) to bottom:

```cpp
optional<Variable> decl_lookup(string name)
```

Returns `nullopt` if not found in any active scope.

```cpp
bool is_global_scope()
```

Returns `true` when only one scope is active (the global scope).

---

## Function registry

Functions are stored in a flat map, separate from scopes:

```cpp
unordered_map<string, Func> function_list;
```

Functions are registered during their own `analyze` call. This means a function must be declared before it can be called — there is no forward declaration pass.

---

## Node analysis

---

### `Let`

1. If an initializer is present, analyzes the expression and extracts its type
2. If no type annotation exists, infers type from the initializer
3. Validates type compatibility between annotation and initializer
4. Enforces that `export` is only used at global scope
5. Sets `is_global`:
   * `1` if `export`
   * `2` (needs check) if at global scope without export
   * `0` otherwise
6. Registers the variable in the current scope

---

### `ExprVar`

1. Looks up the variable name in the scope stack
2. Emits `VARIABLE_NOT_DEFINED` if not found
3. Sets `let` pointer to the declaration node
4. Sets `ty` to the variable's type

---

### `ExprAssign`

1. Analyzes the target variable
2. Applies an implicit cast by setting the expression's type to match the variable
3. Delegates to the expression's `analyze`

---

### `ExprCall`

1. Looks up the callee name in `function_list`
2. Emits `FUNCTION_NOT_DEFINED` if not found
3. Analyzes all argument expressions
4. Validates argument count:
   * exact match for non-variadic functions
   * at-least match for variadic functions
5. Validates argument types against the function signature
6. Sets `ty` to the function's return type

---

### `ExprOp`

1. Analyzes both sides
2. Validates type compatibility between left and right
3. Assigns result type:
   * comparison operators (`>`, `<`, `>=`, `<=`, `==`, `!=`) → `bool`
   * arithmetic operators → type of the left-hand side
4. Applies implicit cast to right-hand side to match left

---

### `ExprAddress` / `ExprDeref`

Both analyze their inner `ExprVar`, then adjust the type:

```cpp
ExprAddress: ty = var.type.ref()    // adds one pointer level
ExprDeref:   ty = var.type.deref()  // removes one pointer level
```

---

### `ExprStructParam`

1. Looks up the struct variable by `struct_name`
2. Resolves field index via `struct_parameter_index(name)`
3. Emits `VARIABLE_NOT_DEFINED` if struct or field is missing
4. Sets `index` for use during codegen
5. Sets `ty` to the struct's type (not the field type — codegen extracts the field type via index)

---

### Literals

All literals assign a fixed type during analysis:

```text
ExprLitInt      → i8 / i16 / i32 / i64  (by value range)
ExprLitFloat    → f32
ExprLitString   → char^
ExprLitChar     → char
```

Integer literals are sized to the smallest type that can hold their value.

---

### `Function`

1. Opens a new scope
2. Analyzes all arguments — each is added to the scope as a `Variable`
3. Analyzes the body block
4. If the block does not terminate (returns `false`) and the return type is non-void, emits a warning:

```text
Control reached end of non-void function
```

5. Closes the scope
6. Registers the function handle in `function_list`

---

### `ExternFn`

Skips body analysis (there is none). Extracts argument types directly from parsed `Argument` nodes and registers the function handle.

---

### `Argument`

Registers itself as a `Variable` in the current scope. Used during `Function::analyze` to populate the function's inner scope.

---

### `Block`

1. Opens a new scope
2. Iterates nodes, tracking whether control flow terminates (`finishes`)
3. Once a terminating node is found, all subsequent nodes are:
   * marked `is_dead = true`
   * warned once: `"Control flow will never reach this statement"`
4. Closes the scope
5. Returns whether the block guarantees termination

---

### `Return`

Analyzes its expression and returns `true` — signaling to `Block` that control flow terminates here.

---

### `If` / `Else`

`If::analyze` only analyzes its body — the condition is not currently analyzed here.

`Else::analyze` analyzes the condition, then both branches. Returns `true` only if both branches terminate.

---

### `While` / `Do`

Both analyze their condition and body. Neither is considered to guarantee termination — both return `false`.

---

### `Struct`

Registers itself as a global `Variable` so it can be looked up by name. The type is already parsed at this point.

---

## Control flow tracking

The `bool` return from `Stmt::analyze` propagates termination status up through the tree:

```text
Return              → true  (always terminates)
Block               → true  (if any node terminates)
Else                → true  (if both branches terminate)
If / While / Do     → false (not guaranteed to terminate)
Let / ExternFn      → false
Function            → true  (registers itself)
```

This is used to:

* detect unreachable code within blocks
* warn on non-void functions that may fall through

---

## Tracing / diagnostic logging

The analyzer has an optional structured logging mode:

```cpp
void Analyzer::trace(
    unordered_map<string, string> data,
    Error::ErrorCodes code,
    Span span
)
```

When `dump_self` is `true`, trace calls are recorded into a `Logger`. Each entry stores key-value pairs, an error code, and a source span.

### `Logger`

```cpp
class Logger {
    vector<LogEntry> entries;

    void log(keys, code, span)
    string to_json()
}
```

Output is a JSON diagnostics array:

```json
{
  "diagnostics": [
    {
      "kind": "search",
      "search": "my_var",
      "for": "var",
      "status": "pass",
      "code": "0",
      "location": { "file": "main.gr", "line": 4, "column": 5 }
    }
  ]
}
```

Enabled via:

```cpp
Analyzer analyzer(/*dump=*/true);
analyzer.dump();
```

Trace calls are present throughout analysis for variable lookups, function searches, declarations, type inference, and struct field resolution.

---

## Type compatibility

```cpp
bool Analyzer::type_is_compatible(Type lhs, Type rhs)
```

Delegates to `Type::is_compatible`. Two types are compatible if they are in the same family (both int, both float, both pointer, or same record).

Used in `ExprOp`, `ExprCall`, and `Let` to validate operations and assignments.

---

## Invariants

The analyzer guarantees that after a successful pass:

* All expressions have a type assigned (`Expr::ty` is non-null)
* All `ExprVar::let` pointers are resolved to their declaration node
* All `ExprStructParam::index` values are valid
* All dead nodes are marked `is_dead = true`
* All referenced functions exist in `function_list`
* All referenced variables exist in some scope at the point of use

---

## Design notes

* The analyzer is a **single forward pass** — no pre-pass for forward declarations
* `is_global` on `Let` uses a three-value flag (`0/1/2`) rather than a bool, allowing deferred resolution of whether a variable ends up in global scope without `export`
* Implicit casting is done by calling `set_type` on an expression before analyzing it, rather than inserting a cast node
* The `Logger` uses `unordered_map<string, string>` for trace data — key order in JSON output is not guaranteed

---

## Limitations

* No forward declarations — functions must be defined before use
* `If` does not analyze its condition expression
* Type inference is one-directional (rvalue → lvalue only)
* No cross-function analysis or inlining hints

---

## Future work

* Forward declaration pre-pass
* Full condition analysis in `If`
* Bidirectional type inference
* Constant folding integration (the `Optimizer::fold` call is currently commented out in `ExprOp`)
* Richer control flow analysis (loop exit paths, exhaustive match)