#pragma once

#include <llvm/IR/Value.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

/// @brief Generates an inline assembly instruction for a syscall to exit
/// @param ctx The LLVM context
/// @return The inline assembly instruction
llvm::InlineAsm* syscall_exit(llvm::LLVMContext& ctx);
