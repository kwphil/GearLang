#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>

// Just generates an int constant and returns it
unique_ptr<Value> Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return std::make_unique<Value>(
        llvm::ConstantInt::get(
            ty->to_llvm(ctx),
            this->value,
            true
        ),
        ty->to_llvm(ctx),
        false
    );
}

// TODO
unique_ptr<Value> Ast::Nodes::ExprLitFloat::generate(Context& ctx) {
    return std::make_unique<Value>(
        llvm::ConstantFP::get(
            llvm::Type::getDoubleTy(ctx.llvmCtx),
            this->value
        ),
        llvm::Type::getDoubleTy(ctx.llvmCtx),
        false
    );
};

// Converts a string into an array of constant i8s
// Creates a constant array using that
// Returns a pointer to the string
unique_ptr<Value> Ast::Nodes::ExprLitString::generate(Context& ctx) {
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

    return std::make_unique<Value>(
        val,
        arr_i8,
        false
    );
}
