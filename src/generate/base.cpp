#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <stdexcept>
#include <memory>

#include "../ast/base.hpp"
#include "../ast/stmt.hpp"
#include "../ast/expr.hpp"
#include "../error.hpp"
#include "../func.hpp"
#include "../sem.hpp"

#include <iostream>

// Checks if the variable already exists
// If it does, it throws an error and quits
// Otherwise, it creates the variable in the current scope
// If in the global scope, it creates a global variable
void Ast::Nodes::Let::generate(Context& ctx) {
    Expr* rvalue = dynamic_cast<Expr*>(expr.get());
    
    if(!rvalue) {
        Error::throw_error(
            line_number,
            target.c_str(),
            "Expected an rvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    
    Value* initVal = rvalue->generate(ctx);

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
        // Creating a placeholder and then assigning the value
        llvm::Constant* placeholder = 
            llvm::Constant::getNullValue(initVal->ty);
        
        // Create the variable
        llvm::GlobalVariable* var = new llvm::GlobalVariable(
            *(ctx.module),
            initVal->ty,
            false,
            llvm::GlobalValue::ExternalLinkage,
            placeholder,
            target
        );

        ctx.bind(target, new Value { 
            .ir=var, 
            .ty=initVal->ty, 
            .is_address=initVal->is_address 
        });

        ctx.builder.CreateStore(initVal->ir, var);
        return;
    }

    // FUNCTION SCOPE
    llvm::Function* fn = ctx.current_fn;
    llvm::AllocaInst* alloca =
        ctx.create_entry_block(fn, target, initVal->ir->getType());

    ctx.bind(target, new Value {
        .ir=alloca,
        .ty=initVal->ty,
        .is_address=initVal->is_address
    });
}

// Creates a new scope, generates all the expressions inside the block,
// then pops the scope
Value* Ast::Nodes::ExprBlock::generate(Context& ctx) {
    ctx.push_scope();
    for (auto& expr : nodes)
        generate_node(expr.get(), ctx);
    ctx.pop_scope();
    return nullptr;
}

// Generates the return value, ensures it is i32
// Then creates a syscall to exit with that return value
void Ast::Nodes::Return::generate(Context& ctx) {
    Expr* exp = dynamic_cast<Expr*>(expr.get());
    Value* retVal = exp->generate(ctx);

    ctx.builder.CreateRet(retVal->ir);
    return;
        
}

// Generates the if condition, creates blocks for the if and the continuation
// Generates the 'then' block inside the if block
// Jumps to the continuation block afterwards   
void Ast::Nodes::If::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition
    Expr* cond_expr = dynamic_cast<Expr*>(cond_expr);
    if(!cond_expr) {
        Error::throw_error(
            line_number,
            "if",
            "Expected rvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    Value* condVal = cond_expr->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal->ir,
        llvm::ConstantInt::get(
            condVal->ir->getType(), // Pointers would be handled different so get the raw type
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, then_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);

    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // continuation
    ctx.builder.SetInsertPoint(then_block);
}

void Ast::Nodes::Else::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* else_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.else", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition
    Expr* cond_expr = dynamic_cast<Expr*>(cond.get());
    if(!cond_expr) {
        Error::throw_error(
            line_number,
            "else",
            "Expected an rvalue but received an lvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    Value* condVal = cond_expr->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal->ir,
        llvm::ConstantInt::get(
            condVal->ir->getType(),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, else_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // else
    ctx.builder.SetInsertPoint(else_block);
    Expr* else_expr2 = dynamic_cast<Expr*>(expr.get());
    if(!else_expr2) {
        Error::throw_error(
            line_number,
            "",
            "Expected rvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    else_expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // end
    ctx.builder.SetInsertPoint(then_block);
}

#include <iostream>

void generate_node(Ast::Nodes::NodeBase* node, Context& ctx) {
    using namespace Ast::Nodes;
    
    if (auto* stmt = dynamic_cast<Stmt*>(node)) {
        stmt->generate(ctx);
    } else if (auto* expr = dynamic_cast<Expr*>(node)) {
        expr->generate(ctx);
    } else {
        throw std::runtime_error("invalid node");
    }
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& node : content) {
        generate_node(node.get(), ctx);
    }
}
