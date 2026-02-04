#include <llvm/IR/Value.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

#include <format>

#include "ctx.hpp"
#include "syscall.hpp"

llvm::InlineAsm* syscall_exit(llvm::LLVMContext& ctx) {
    // Linux x86-64: exit(int status)
    // rdi = status
    // rax = 60
    return llvm::InlineAsm::get(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(ctx),
            { llvm::Type::getInt32Ty(ctx) },
            false
        ),
        "mov $$60, %rax\n"
        "syscall",
        "{rdi},~{rax},~{rcx},~{r11},~{memory}",
        true
    );
}

llvm::InlineAsm* align_stack(llvm::LLVMContext& ctx) {
    // Aligning the stack for glibc
    return llvm::InlineAsm::get(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(ctx),
            false
        ),
        "andq $$-16, %rsp",
        "~{rsp},~{memory}",
        true,
        true
    );
}

llvm::Value* get_rsp(Context& ctx) {
    auto i64 = llvm::Type::getInt64Ty(ctx.llvmCtx);
    auto get_reg = llvm::InlineAsm::get(
        llvm::FunctionType::get(
            i64,
            false
        ),
        "movq %rsp, $0",
        "=r",
        false
    );

    return ctx.builder.CreateAlloca(i64, 
            ctx.builder.CreateCall(get_reg)
        );
}