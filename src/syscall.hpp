#pragma once

#include <llvm/IR/Value.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

llvm::InlineAsm* syscall_exit(llvm::LLVMContext& ctx);