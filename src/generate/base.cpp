#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <stdexcept>

#include "../ast/stmt.hpp"
#include "../ast/expr.hpp"
#include "../error.hpp"
#include "../func.hpp"

// Checks if the variable already exists
// If it does, it throws an error and quits
// Otherwise, it creates the variable in the current scope
// If in the global scope, it creates a global variable
llvm::Value* Ast::Nodes::Let::generate(Context& ctx) {
    llvm::Value* initVal = expr->generate(ctx);

    // Making sure that the variable doesn't exist
    auto _var = ctx.lookup(target);
    
    if(_var) {
        Error::throw_error(
            line_number,
            target.c_str(),
            "Variable already defined.",
            Error::ErrorCodes::VARIABLE_ALREADY_DEFINED
        );
    }

    // GLOBAL SCOPE
    llvm::Function* _fn = ctx.current_fn;
    if (_fn->getName() == ".global_fn") {
        llvm::Type* ty = initVal->getType();

        // Creating a placeholder and then assigning the value
        llvm::Constant* placeholder = 
            llvm::Constant::getNullValue(ty);
        
        // Create the variable
        llvm::GlobalVariable* var = new llvm::GlobalVariable(
            *(ctx.module),
            ty,
            false,
            llvm::GlobalValue::ExternalLinkage,
            placeholder,
            target
        );

        ctx.bind(target, var);

        ctx.builder.CreateStore(initVal, var);

        return var;
    }

    // FUNCTION SCOPE
    llvm::Function* fn = ctx.current_fn;
    llvm::AllocaInst* alloca =
        ctx.create_entry_block(fn, target, initVal->getType());

    ctx.bind(target, alloca);
    return alloca;
}

// Creates a new scope, generates all the expressions inside the block,
// then pops the scope
llvm::Value* Ast::Nodes::ExprBlock::generate(Context& ctx) {
    ctx.push_scope();
    for (auto& expr : nodes)
        expr->generate(ctx);
    ctx.pop_scope();
    return nullptr;
}

// Generates the return value, ensures it is i32
// Then creates a syscall to exit with that return value
llvm::Value* Ast::Nodes::Return::generate(Context& ctx) {
    llvm::Value* retVal = expr->generate(ctx);

    if(
        ctx.module->getFunction(".global_fn") != ctx.current_fn
        && ctx.module->getFunction("main") != ctx.current_fn
    ) {
        ctx.builder.CreateRet(retVal);
        return retVal;
    }
    
    llvm::Function* exit = declare_func(
        llvm::Type::getVoidTy(ctx.llvmCtx),
        { llvm::Type::getInt32Ty(ctx.llvmCtx) },
        "exit", ctx, false
    );
    
    ctx.builder.CreateCall(exit, { retVal });
    ctx.builder.CreateUnreachable();
    return retVal;
}

// Generates the if condition, creates blocks for the if and the continuation
// Generates the 'then' block inside the if block
// Jumps to the continuation block afterwards   
llvm::Value* Ast::Nodes::If::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition ONCE
    llvm::Value* condVal = cond->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            condVal->getType(),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, then_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    expr->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // continuation
    ctx.builder.SetInsertPoint(then_block);

    return nullptr;
}

llvm::Value* Ast::Nodes::Else::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* else_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.else", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition ONCE
    llvm::Value* condVal = cond->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            condVal->getType(),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, else_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    expr->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // else
    ctx.builder.SetInsertPoint(else_block);
    else_expr->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // end
    ctx.builder.SetInsertPoint(then_block);

    return nullptr; 
}

#include <iostream>

void Ast::Program::generate(Context& ctx) {
    for (const auto& node : content) {
        if (auto* stmt = dynamic_cast<Stmt*>(node.get())) {
            stmt->generate(ctx);
        } else if (auto* expr = dynamic_cast<Expr*>(node.get())) {
            // Top-level expressions: generate for side effects, discard value
            expr->generate(ctx);
        } else {
            throw std::runtime_error("invalid node");
        }
    }
}
