#pragma once

#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include "ctx.hpp"

// Just some quick short hands

llvm::Function* declare_func(
    llvm::Type* ret_type,
    llvm::ArrayRef<llvm::Type*> args,
    const char* name, Context& ctx,
    bool variadic
);