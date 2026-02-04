#pragma once

#include "ctx.hpp"

#include <llvm/IR/Value.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

/// @brief Generates an inline assembly instruction for a syscall to exit
/// @param ctx The LLVM context
/// @return The inline assembly instruction
llvm::InlineAsm* syscall_exit(llvm::LLVMContext& ctx);

/// @brief Generates inline assembly to align the stack for glibc
/// @param ctx The LLVM context
/// @return the inline assembly instruction
llvm::InlineAsm* align_stack(llvm::LLVMContext& ctx);

/// @brief Gets a register and passes it to a variable
/// @param ctx The LLVM context
/// @param reg The register in AT&T syntax
llvm::Value* get_rsp(Context& ctx);