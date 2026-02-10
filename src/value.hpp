#pragma once // To remove circular dependencies

#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>

typedef struct {
    llvm::Value* ir;
    llvm::Type* ty;
    bool is_address;
} Value;