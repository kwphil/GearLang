#pragma once // To remove circular dependencies

#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>

typedef struct {
    llvm::Value* ir;
    llvm::Type* ty;
    int addr; // number of pointers to pointers. 0 means a raw value
} Value;