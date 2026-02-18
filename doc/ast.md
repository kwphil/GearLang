# AST

## Design Goals

* Represent source code structure explicitly
* Separate parsing, pretty-printing, and code generation
* Use polymorphism for extensibility
* Generate LLVM IR directly from AST nodes

## Base Node

```cpp
class NodeBase {
public:
    virtual llvm::Value* generate(Context&) = 0;
    static std::unique_ptr<NodeBase> parse(Lexer::Stream&);
};
```

All AST nodes:

* Can be parsed from tokens
* Can render themselves as source
* Can generate LLVM IR

## Expressions

```cpp
class Expr : public NodeBase
```

Expressions are a subset of nodes that evaluate to values.

### Parsing Strategy

* `parseExpr()` – Handles binary operators
* `parseTerm()` – Handles literals, variables, parentheses

Operator precedence is currently flat (left–right, no hierarchy).

### Binary Operations 
```cpp
enum Type { Add, Sub, Mul, Div };
```

Represents arithmetic expressions like:

`(a + b)`
`(x * y)`

#### Code Generation

* Generates integer arithmetic LLVM instructions
* Allocates temporary storage
* Returns a loaded value

### Literals

#### Integer Literal 

* Parsed via std::stoi
* Emits llvm::ConstantInt

#### Float Literal 
* Parsed via std::stod
* Code generation is not yet implemented

### Variables 

* Represents variable access
* Looks up variables in Context::named_values
* Emits a load instruction

#### Assignment 
x = expression

* Requires variable to already exist
* Currently missing store logic in generate()

#### Let Binding 

let x = expression;

##### Responsibilities:

* Allocate stack storage in function entry block
* Initialize variable
* Register variable in context symbol table

### Return Statement
return expression;

Implementation details:

* Evaluates return value
* Ensures i32 width
* Emits Linux syscall exit via inline assembly
* Terminates control flow with unreachable

**This bypasses normal LLVM ret to directly exit the process.**

### If Expression 

`if condition expression`

* Generates conditional branches
* Converts integer condition to boolean via != 0
* Uses basic blocks (if, then)

**No else support yet**

## Program
```cpp
Program Node (Ast::Program)
class Program {
    std::vector<std::unique_ptr<NodeBase>> content;
};
```

* Represents the entire source file.

### Responsibilities

* Parse all top-level statements
* Pretty-print program
* Drive code generation sequentially