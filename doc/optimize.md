# GearLang compiler: Optimizer

The optimizer is an AST-level transformation pass that runs after analysis.

It reduces constant expressions to literals before code generation.

---

## Design overview

The optimizer is:

* **AST-based** (no separate IR)
* **opt-in** (transformations are gated by a bitmask)
* **recursive** (folds nested expressions bottom-up)
* **non-destructive of types** (types are preserved across folds)

---

## Optimization flags

Available optimizations are defined as bitmask constants:

```cpp
#define OPERATION_FOLDING     0x1   // 1 + 1 => 2
#define DEAD_CODE_ELIMINATION 0x2   // removes unreachable nodes
```

The active set is stored globally:

```cpp
int Optimizer::opts;
```

Checked via:

```cpp
constexpr bool is_opt_active(unsigned char opt)
```

---

## Entry points

```cpp
unique_ptr<NodeBase> Ast::optimize(unique_ptr<NodeBase> node)
unique_ptr<Expr> Ast::optimize_expr(unique_ptr<Expr> node)
```

`optimize` is the general entry point. It checks whether the node is an expression, and if so, delegates to `optimize_expr`. Non-expression nodes are passed through unchanged.

`optimize_expr` checks for `ExprOp` specifically and forwards to `Optimizer::fold`. All other expression types are passed through.

---

## Constant folding

```cpp
unique_ptr<Expr> Optimizer::fold(unique_ptr<ExprOp> expr)
```

Folds a binary operation if both operands are constants.

### Process

1. Check if left and right operands are constants via `is_const`
2. Recursively attempt to fold each side:
   * if a side is constant, run it through `optimize_expr` first (handles nested ops)
   * if not constant, leave it as-is
3. If both sides are still not fully constant after recursion, return a rebuilt `ExprOp` with the (partially folded) children
4. If both sides are constant, evaluate and return a new literal

### Folding example

```gr
1 + 2 + 3
```

The inner `1 + 2` folds to `ExprLitInt(3)`, then the outer `3 + 3` folds to `ExprLitInt(6)`.

---

### Constant evaluation

Float operands:

```cpp
ExprLitFloat(l->value + r->value)
```

Integer operands:

```cpp
ExprLitInt(l->value + r->value)
```

The result node inherits the type of the original `ExprOp`:

```cpp
new_obj->set_type(*expr->get_type())
```

### Current limitation

Only addition is implemented — the operator type (`expr->type`) is not consulted. All folds use `+` regardless of the actual operator.

---

## Constness check

```cpp
bool Optimizer::is_const(Expr* expr)
```

Returns `true` if the expression is a compile-time constant.

### Rules

```text
ExprLitInt      → true
ExprLitFloat    → true
ExprOp          → true if both children are const (recursive)
anything else   → false
```

`ExprLitString`, `ExprLitChar`, variables, calls, and all other expressions are not considered constant.

---

## Invariants

The optimizer guarantees:

* Types are preserved — every folded node calls `set_type` with the original type
* Non-constant subtrees are returned unchanged
* Folding is bottom-up — children are reduced before their parent

---

## Design notes

* `Ast::optimize` and `Ast::optimize_expr` use raw `cast_to` casts followed by a `release()` — ownership is manually transferred. This is fragile if `cast_to` returns a non-owning pointer to a node whose `unique_ptr` is still live.
* The optimizer currently has no integration point in the main compilation pipeline — `Optimizer::fold` is called directly from `ExprOp::analyze`, but that call is commented out.
* Dead code elimination is defined as a flag but has no implementation — nodes are marked `is_dead` by the analyzer, but the optimizer does not act on this yet.

---

## Limitations

* Only `ExprOp` nodes are optimized — no folding of unary expressions, casts, or conditionals
* Only addition is correctly folded — the operator is ignored
* `ExprLitString` and `ExprLitChar` are never constant-folded
* Dead code elimination flag exists but is not implemented
* No integration into the main compile pipeline

---

## Future work

* Respect the operator type in `fold` (`-`, `*`, `/`, comparisons)
* Constant folding for all literal types
* Dead code elimination pass (walk AST, remove `is_dead` nodes)
* Integrate optimizer into the pipeline with configurable opt levels
* Strength reduction (e.g. `x * 2` → `x + x` or shift)