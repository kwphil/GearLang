#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include "../ast.hpp"

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
llvm::Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);

    if(lhs->getType()->isDoubleTy()) {
        switch(type) {
            case Add: return ctx.builder.CreateFAdd(lhs, rhs, "faddtmp");
            case Sub: return ctx.builder.CreateFSub(lhs, rhs, "fsubtmp");
            case Mul: return ctx.builder.CreateFMul(lhs, rhs, "fmultmp");
            case Div: return ctx.builder.CreateFDiv(lhs, rhs, "fdivtmp");
        }
    }

    switch (type) {
        case Add: return ctx.builder.CreateAdd(lhs, rhs, "iaddtmp");
        case Sub: return ctx.builder.CreateSub(lhs, rhs, "isubtmp");
        case Mul: return ctx.builder.CreateMul(lhs, rhs, "imultmp");
        case Div: return ctx.builder.CreateSDiv(lhs, rhs, "idivtmp");
    }

    throw std::runtime_error("Invalid ExprOp");
}

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise:
// Checks if it is a global and returns it
llvm::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    llvm::Value* var = ctx.lookup(name);
    if (!var)
        throw std::runtime_error("Unknown variable: " + name);

    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        return ctx.builder.CreateLoad(
            gv->getValueType(),
            gv,
            name + ".load"
        );
    }

    return ctx.builder.CreateLoad(
        var->getType(),
        var,
        name + ".load"
    );
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
llvm::Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    llvm::Value* alloca = ctx.lookup(name);
    if (!alloca)
        throw std::runtime_error("Unknown variable: " + name);

    llvm::Value* value = expr->generate(ctx);
    ctx.builder.CreateStore(value, alloca);

    return value;
}