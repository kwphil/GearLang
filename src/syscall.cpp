#include <llvm/IR/Value.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>

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