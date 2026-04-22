# Context

## Purpose

Context owns all LLVM-related state and utilities.

## Core LLVM Objects

`llvm::LLVMContext llvmCtx;`
`llvm::IRBuilder<> builder;`
`std::unique_ptr<llvm::Module> module;`

* LLVMContext – Global LLVM state
* IRBuilder – Instruction emission helper

## Module – Output container for IR

Entry Function Tracking

`std::shared_ptr<llvm::Function*> mainFn;`

Used by AST nodes (e.g. If) to access the current function.

## Symbol Table

std::unordered_map<std::string, llvm::Value*> named_values;

Maps variable names to stack allocations (alloca).

* Utility Functions
* Stack Allocation
* llvm::AllocaInst* create_entry_block(...)

Ensures:

* All alloca instructions live in the function entry block
* Proper LLVM optimization behavior

## IR Rendering
```cpp
std::string render();
```

Converts LLVM IR into a printable string.