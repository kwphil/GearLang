#include "func.hpp"
#include "ctx.hpp"

llvm::Function* declare_func(
    llvm::Type* ret_type,
    llvm::ArrayRef<llvm::Type*> args,
    const char* name, Context& ctx,
    bool variadic
) {
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ret_type,
        args,
        variadic
    );

    return llvm::Function::Create(
        fn_type,
        llvm::GlobalValue::ExternalLinkage,
        name, *ctx.module
    );
}