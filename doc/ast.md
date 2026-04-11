# GearLang compiler: AST (Abstract Syntax Tree)

The AST is the core representation of a GearLang program.

Every stage after parsing operates on AST nodes:

* Analyzer
* Optimizer
* Codegen

The AST is not just structural—it is also:

* semantically enriched (types, symbol links)
* executable (LLVM generation happens directly from nodes)

---

## Design overview

The AST is:

* **class-based** (inheritance hierarchy)
* **owned via `unique_ptr`**
* **mutable across passes**
* **annotated during analysis**

Every construct in the language maps to a node.

---

## Core base node

All nodes inherit from:

```cpp
NodeBase
```

### Structure

```cpp id="sjnn4y"
class NodeBase {
  const Span span_meta;
  bool is_dead = false;

  virtual std::string to_string() = 0;
  virtual llvm::Value* generate(Context& ctx) = 0;
};
```

---

### Responsibilities

Every node must:

* Provide debug output (`to_string`)
* Generate LLVM (`generate`)
* Carry source location (`Span`)
* Support parsing (via static `parse`)

---

### `is_dead`

Used by optimization passes.

If `true`, the node:

* should not generate code
* may be removed later

---

## Program node

Top-level container:

```cpp id="4qpnr3"
class Program {
  std::deque<std::unique_ptr<NodeBase>> content;
};
```

### Notes

* Uses `deque` (not vector)
* Nodes are pushed to the **front**

```cpp id="uvb3bg"
content.push_front(...)
```

This implies:

* reverse construction order during parsing
* later stages must respect ordering

---

## Node categories

Nodes are split into two major groups:

```text id="ru2z7t"
NodeBase
 ├── Expr   (returns a value)
 └── Stmt   (does not return a value)
```

---

# Expressions (`Expr`)

Expressions produce values and participate in type inference.

---

## Base class

```cpp id="f4o6tm"
class Expr : public NodeBase {
  unique_ptr<Sem::Type> ty;
};
```

---

### Type handling

Expressions store their type:

```cpp id="7lqk0c"
optional<Sem::Type> get_type()
void set_type(Sem::Type)
```

### Important

* Types are **not guaranteed during parsing**
* Types are assigned during **analysis**
* `get_type()` may return `nullopt` before analysis

---

### Analysis contract

Every expression must implement:

```cpp id="74uzc7"
unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer&)
```

This:

* resolves types
* validates semantics
* returns metadata for evaluation

---

## Expression types

---

### Binary operations

```cpp id="6d0yzf"
ExprOp {
  Type type;
  Expr left, right;
}
```

Supports:

```text id="9b1c8r"
+  -  *  /  >  <  >=  <=  ==  !=
```

---

### Function calls

```cpp id="9drtsm"
ExprCall {
  string callee;
  vector<Expr> args;
}
```

Notes:

* callee is stored as string (resolved later)
* arguments are full expressions

---

### Literals

Base:

```cpp id="e8n7p0"
Literal : Expr {
  llvm::Type* cast_type;
}
```

Derived:

* `ExprLitInt`
* `ExprLitFloat`
* `ExprLitString`
* `ExprLitChar`

---

### Variable system

#### Variable reference

```cpp id="9v4dfg"
ExprVar {
  string name;
  NodeBase* let;
}
```

Key detail:

* `let` is resolved during analysis
* links variable usage → declaration

---

#### Assignment

```cpp id="q9b4dr"
ExprAssign {
  ExprVar var;
  Expr expr;
}
```

---

#### Address-of

```cpp id="x5q4cy"
ExprAddress
```

Implements:

```gr id="p6vxg7"
#x
```

---

#### Dereference

```cpp id="9kwm7h"
ExprDeref
```

Implements:

```gr id="lm3zsf"
@x
```

---

#### Struct field access

```cpp id="1bq7m3"
ExprStructParam
```

Represents:

```gr id="92cv9p"
s.x
```

Internally:

* stores struct name
* resolves field index

---

#### Function arguments

```cpp id="wr5rjw"
Argument : ExprVar
```

Notes:

* behaves like a variable
* also carries LLVM binding

---

# Statements (`Stmt`)

Statements do not return values.

They define structure and control flow.

---

## Base class

```cpp id="1b1kcl"
class Stmt : public NodeBase {
  virtual bool analyze(Sem::Analyzer&) = 0;
};
```

### Analysis returns `bool`

Used to:

* indicate control flow validity
* propagate errors

---

## Statement types

---

### Variable declaration

```cpp id="0h2y2q"
Let {
  string target;
  Expr expr;
  Type ty;
}
```

Additional flags:

```cpp id="0g9vnb"
bool is_global;
bool is_public;
```

Also stores:

```cpp id="mqk1kz"
llvm::Value* var;
```

---

### Return

```cpp id="t0ztg9"
Return {
  Expr expr;
}
```

---

### Block

```cpp id="xq7p6k"
Block {
  vector<NodeBase> nodes;
}
```

Represents:

```gr id="q3uxbq"
{
  ...
}
```

---

### If / Else

```cpp id="6q9o6f"
If {
  Expr cond;
  NodeBase expr;
}
```

```cpp id="s7qz8h"
Else : If {
  NodeBase else_expr;
}
```

---

### Loops

#### While

```cpp id="cv9v7n"
While {
  NodeBase code;
  Expr cond;
}
```

#### Do

```cpp id="xj8b8k"
Do {
  NodeBase code;
  Expr cond;
}
```

---

### Struct definition

```cpp id="h7r8wr"
Struct {
  string name;
  Type ty;
}
```

Codegen happens immediately via:

```cpp id="sx9r7q"
ty.struct_to_llvm(...)
```

---

### Function definition

```cpp id="f94nl2"
Function {
  string name;
  Type ty;
  deque<Argument> args;
  NodeBase block;
}
```

Flags:

```cpp id="jz7r3u"
bool is_variadic;
bool is_public;
```

---

### External function

```cpp id="r4g7tx"
ExternFn {
  string callee;
  Type ty;
  deque<Argument> args;
}
```

Supports:

* FFI
* optional no-mangle

---

### Include (FFI hook)

```cpp id="y0k5lf"
Include {
  string lang;
  string type;
  string file;
}
```

### Important behavior

* Initializes FFI backend
* Registers headers immediately

```cpp id="3a2n2g"
ffi_list[lang]->add_header(file);
```

---

# Parsing model

Each node implements:

```cpp id="q8qg7c"
static parse(Lexer::Stream&)
```

Parsing is:

* distributed across node types
* not centralized

---

## Entry point

```cpp id="i3l2f7"
NodeBase::parse(Stream&)
```

Dispatches based on:

* token type
* keywords

---

## Expression parsing

Handled via:

```cpp id="3ycb2y"
Expr::parseExpr
Expr::parseTerm
```

This implies:

* precedence-based parsing
* recursive descent

---

# Analysis phase interaction

During analysis:

* types are assigned (`Expr::ty`)
* variables are resolved (`ExprVar::let`)
* scopes are enforced
* semantic errors are emitted

---

# Code generation

Each node directly emits LLVM:

```cpp id="xjzq8v"
llvm::Value* generate(Context&)
```

Notes:

* expressions return values
* statements may return `nullptr`
* control flow nodes emit branches

---

## Helper

```cpp id="2clw9s"
generate_node(NodeBase*, Context&)
```

Used to:

* safely emit nodes
* skip dead nodes

---

# Optimization

Optimization is AST-based:

```cpp id="b6p5p1"
Ast::optimize(...)
Ast::optimize_expr(...)
```

Works by:

* transforming nodes
* marking nodes as dead
* replacing subtrees

---

# Invariants

The AST guarantees:

* All nodes have valid spans
* Ownership is strictly `unique_ptr`
* Expressions may be untyped before analysis
* After analysis:

  * all expressions have types
  * all symbols are resolved

---

# Design notes

* AST is both **semantic IR** and **codegen IR**
* No separate IR layer (yet)
* Strong coupling:

  * AST ↔ analyzer
  * AST ↔ LLVM

This simplifies early development, but may:

* limit optimization flexibility later
* increase node complexity

---

# Future work

* Separate IR layer
* Arena allocation for nodes
* Visitor pattern (optional)
* Better node introspection
