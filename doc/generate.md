# GearLang compiler: Code Generation

Code generation is the final stage of the GearLang compiler.

Each AST node implements:

```cpp
llvm::Value* generate(Context& ctx)
```

Nodes emit LLVM IR directly. There is no intermediate representation between the AST and LLVM.

---

## Design overview

Codegen is:

* **node-driven** (each node owns its own `generate`)
* **direct** (AST → LLVM IR, no separate IR layer)
* **context-threaded** (`Context` carries all mutable LLVM state)
* **dead-code-aware** (respects `is_dead` and `DEAD_CODE_ELIMINATION`)

---

## Context

All codegen operates through a shared `Context`:

```cpp
Context {
    llvm::LLVMContext llvmCtx;
    llvm::IRBuilder<> builder;
    llvm::Module* module;
    llvm::Function* current_fn;
    unique_ptr<llvm::BasicBlock*> main_entry;
    llvm::BasicBlock* global_entry;
}
```

Key fields:

* `builder` — the active IR insertion point
* `current_fn` — the function currently being generated
* `main_entry` — the entry block of `main`, used to restore context after generating nested functions
* `global_entry` — the global-level insertion point, restored after each function

---

## Entry point

```cpp
void Ast::Program::generate(Context& ctx)
```

Iterates all top-level nodes and calls `generate_node` on each.

---

## `generate_node`

```cpp
void generate_node(NodeBase* node, Context& ctx)
```

The central dispatch helper. Used wherever a node needs to be emitted without knowing its type:

1. If `is_dead` and `DEAD_CODE_ELIMINATION` is active → skip
2. Downcasts to `Stmt` or `Expr` via `dynamic_cast`
3. Calls `generate` on the result
4. Throws `std::runtime_error` if neither cast succeeds

Used by `Block`, `Function`, and `Program`.

---

## Statements

---

### `Block`

Iterates its nodes and calls `generate_node` on each. Skips entirely if `is_dead` and dead code elimination is active.

Does not create an LLVM basic block — blocks in GearLang are a structural concept, not a control flow boundary.

---

### `Let`

#### Local variable

1. Generates the initializer expression
2. Creates an `alloca` in the function entry block via `ctx.create_entry_block`
3. Stores `alloca` in `Let::var` for later access
4. Emits a `store` if an initializer is present

#### Global variable

1. Generates the initializer expression
2. If the initializer is a constant, uses it directly
3. If not (e.g. runtime value), initializes to zero and sets `require_store = true`
4. Creates a `llvm::GlobalVariable` with:
   * `ExternalLinkage` if `is_public`
   * `PrivateLinkage` otherwise
5. If `require_store`, emits a store at the current insertion point

---

### `Return`

1. Generates the return expression
2. If the expression type doesn't match the function's return type, emits an `IntCast`
3. Emits `CreateRet`

---

### `If`

Emits three basic blocks:

```text
if.true   → body
if.end    → continuation
```

Sequence:

1. Generate condition, compare with `ICmpNE 0`
2. `CreateCondBr` → `if.true` or `if.end`
3. Emit body in `if.true`, unconditional branch to `if.end`
4. Set insert point to `if.end`

Returns `nullptr`.

---

### `Else`

Emits four basic blocks:

```text
if.true   → true branch
if.else   → false branch
if.end    → continuation
```

Sequence:

1. Generate condition, compare with `ICmpNE 0`
2. `CreateCondBr` → `if.true` or `if.else`
3. Emit true branch, branch to `if.end`
4. Emit false branch, branch to `if.end`
5. Set insert point to `if.end`

---

### `While`

Emits three basic blocks:

```text
while.cond  → condition check
while.body  → loop body
while.end   → after loop
```

Sequence:

1. Unconditional branch to `while.cond`
2. In `while.cond`: generate condition, `CreateCondBr` → body or end
3. In `while.body`: generate body, unconditional branch back to `while.cond`
4. Set insert point to `while.end`

---

### `Do`

Emits three basic blocks:

```text
do.body  → loop body (entered unconditionally first)
do.cond  → condition check
do.end   → after loop
```

Sequence:

1. Unconditional branch to `do.body`
2. In `do.body`: generate body, branch to `do.cond`
3. In `do.cond`: generate condition, `CreateCondBr` → body or end
4. Set insert point to `do.end`

Note: unlike `While`, the body always executes at least once.

---

## Functions

---

### `Function`

1. Collects LLVM types for all arguments
2. If `name == "main"`: looks up the pre-declared `main` function, creates `main_fn` block
3. Otherwise: calls `declare_func` to create the function with appropriate linkage
4. Names each LLVM argument after its AST counterpart
5. Sets insert point to `entry`
6. For each argument: creates an `alloca`, stores the argument value, and saves the `alloca` in `Argument::var`
7. Generates the function body via `generate_node`
8. If the last block has no terminator:
   * `void` functions → `CreateRetVoid`
   * other functions → `CreateRet(null value)`
9. Restores `current_fn` to `main` and insert point to `global_entry`

---

### `ExternFn`

Declares an external function in the module with `ExternalLinkage`.

Does not emit a body. The `no_mangle` flag is noted but not yet implemented.

---

## Expressions

---

### `ExprOp`

Generates both sides, then selects the LLVM instruction based on type and operator:

#### Float operations

```text
+   → CreateFAdd
-   → CreateFSub
*   → CreateFMul
/   → CreateFDiv
==  → CreateFCmpOEQ
!=  → CreateFCmpONE
>   → CreateFCmpOGT
<   → CreateFCmpOLT
>=  → CreateFCmpOGE
<=  → CreateFCmpOLE
```

#### Integer operations

```text
+   → CreateAdd
-   → CreateSub
*   → CreateMul
/   → CreateSDiv
==  → CreateICmpEQ
!=  → CreateICmpNE
>   → CreateICmpSGT
<   → CreateICmpSLT
>=  → CreateICmpSGE
<=  → CreateICmpSLE
```

The dispatch is a `CREATE_OP` macro that forwards to the builder method with a named temp.

---

### `ExprAssign`

1. Gets the alloca of the target variable via `var->access_alloca`
2. Generates the value expression
3. Emits `CreateStore`
4. Returns the stored value

---

### `ExprCall`

1. Looks up the function by name in the module
2. Generates all argument values
3. If the return type is `void` → `CreateCall` with no name
4. Otherwise → `CreateCall` with name `"call"`, returns the result

---

### `ExprVar`

Loads the variable's value from its alloca:

```cpp
deref(ctx, access_alloca(ctx), ty, name, ".load")
```

`access_alloca` resolves the alloca by reading `Let::var` or `Argument::var` through the `let` pointer set during analysis.

---

### `ExprStructParam`

#### `generate`

Loads a specific field of a struct:

1. Gets the field's LLVM type via `ty->struct_param_ty(index)`
2. Loads from the GEP address returned by `access_alloca`

#### `access_alloca`

Computes the field address via `CreateStructGEP`:

```cpp
ctx.builder.CreateStructGEP(struct_ll, get_var(let), index + 1)
```

Note the `index + 1` offset — field indexing is 1-based here.

---

### `ExprAddress`

Returns the alloca directly (the pointer itself) rather than loading from it:

```cpp
return var->access_alloca(ctx);
```

---

### `ExprDeref`

1. Generates the variable to get a pointer value
2. Emits a `CreateLoad` using the dereferenced type

---

## Literals

All literals emit LLVM constants directly.

### `ExprLitInt`

```cpp
llvm::ConstantInt::get(ty, value, /*signed=*/true)
```

### `ExprLitFloat`

```cpp
llvm::ConstantFP::get(ty, value)
```

### `ExprLitChar`

```cpp
llvm::ConstantInt::get(ty, (int)c)
```

### `ExprLitString`

Strings are emitted as private global `i8` arrays with a null terminator:

1. Allocate `chars.size() + 1` slots
2. Fill with `ConstantInt::get(i8, char)` per character
3. Append `'\0'`
4. Create `ConstantArray` of type `[N x i8]`
5. Emit as a `GlobalVariable` with `PrivateLinkage` and name `.str`

The returned value is a pointer to the global array.

---

## Dead code elimination

Both `Block::generate` and `generate_node` check:

```cpp
if(node->is_dead && is_opt_active(DEAD_CODE_ELIMINATION)) return nullptr;
```

Nodes marked `is_dead` by the analyzer are silently skipped when the `DEAD_CODE_ELIMINATION` flag is set.

---

## Invariants

Codegen assumes:

* Analysis has completed — all types are assigned, all `let` pointers are resolved
* No errors are present in the AST — behavior is undefined if errors reached codegen
* Every `ExprVar` has a valid `let` pointer to a `Let` or `Argument` node
* Every struct field access has a valid resolved `index`
* The `main` function is pre-declared in the module before codegen begins

---

## Design notes

* All pointer types map to opaque LLVM pointers — GearLang tracks pointee types internally
* Argument allocas are created in the function entry block (not inline) to satisfy LLVM's `alloca` placement requirements
* `ExternFn` `no_mangle` support is stubbed — the flag is parsed and stored but has no effect at codegen
* `Do::generate` has a redundant unconditional branch to `loop_body` after setting the insert point to `after_loop` — this is a bug

---

## Limitations

* No struct construction syntax — structs must be populated field by field
* String literals are always emitted as new globals — no deduplication
* Integer division always uses signed (`CreateSDiv`) — unsigned division is not supported
* Condition expressions in `If`/`While`/`Do` always use `ICmpNE 0` — no float or pointer condition support

---

## Future work

* Unsigned integer operation support
* String literal deduplication
* Fix `Do` loop terminator bug
* Implement `no_mangle` for `ExternFn`
* Float and pointer condition handling in branches
* GEP index fix for struct fields (audit `index + 1` offset)