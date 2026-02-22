#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>

// Just generates an int constant and returns it
// TODO: Support different bit widths for optimization 
Value* Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return new Value{
        .ir=llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
            this->value,
            true
        ),
        .ty=llvm::Type::getInt32Ty(ctx.llvmCtx),
        .addr=false
    };
}

// TODO
Value* Ast::Nodes::ExprLitFloat::generate(Context& ctx) {
    return new Value{
        .ir=llvm::ConstantFP::get(
            llvm::Type::getDoubleTy(ctx.llvmCtx),
            this->value
        ),
        .ty=llvm::Type::getDoubleTy(ctx.llvmCtx),
        .addr=false
    };
};

// Converts a string into an array of constant i8s
// Creates a constant array using that
// Returns a pointer to the string
Value* Ast::Nodes::ExprLitString::generate(Context& ctx) {
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

    llvm::Value* val = new llvm::GlobalVariable(
        *ctx.module,
        arr_i8,
        true,
        llvm::GlobalValue::ExternalLinkage,
        array,
        ".str"
    );

    return new Value {
        .ir=val,
        .ty=arr_i8,
        .addr=false
    };
}
