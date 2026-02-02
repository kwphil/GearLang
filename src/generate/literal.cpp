#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include "../ast.hpp"

// Just generates an int constant and returns it
// TODO: Support different bit widths for optimization 
llvm::Value* Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        this->value,
        true
    );
}

// TODO
llvm::Value* Ast::Nodes::ExprLitFloat::generate(Context& ctx) {
    return llvm::ConstantFP::get(
        // TODO: Support different float types for optimization
        llvm::Type::getDoubleTy(ctx.llvmCtx),
        this->value
    );
};

// Converts a string into an array of constant i8s
// Creates a constant array using that
// Returns a pointer to the string
llvm::Value* Ast::Nodes::ExprLitString::generate(Context& ctx) {
    std::vector<llvm::Constant*> chars(string.size());
    llvm::Type* i8 = llvm::Type::getInt8Ty(ctx.llvmCtx);
    llvm::ArrayType* arr_i8 = llvm::ArrayType::get(i8, chars.size());

    for(int i = 0; i < string.size(); i++) {
        chars[i] = llvm::ConstantInt::get(i8, string[i]);
    }

    auto array = llvm::ConstantArray::get(
        arr_i8, 
        chars
    );

    return new llvm::GlobalVariable(
        *ctx.module,
        arr_i8,
        true,
        llvm::GlobalValue::ExternalLinkage,
        array,
        ".str"
    );
}