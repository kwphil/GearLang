#include <stdexcept>
#include <llvm/IR/Type.h>
#include "var.hpp"
#include "ctx.hpp"

llvm::Type* Ast::type_to_llvm_type(Type ty, Context& ctx) {
    switch(ty) {
        case(Type::I32):  return llvm::Type::getInt32Ty(ctx.llvmCtx);
        case(Type::F32):  return llvm::Type::getFloatTy(ctx.llvmCtx);
        case(Type::Void): return llvm::Type::getVoidTy(ctx.llvmCtx);
        case(Type::Invalid): return nullptr;
        default: throw std::runtime_error("You shouldn't be here!!!!!!");
    }
} 
