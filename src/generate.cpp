#include "ast.hpp"

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>

#include "syscall.hpp"

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
llvm::Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);

    switch (type) {
        case Add: return ctx.builder.CreateAdd(lhs, rhs, "addtmp");
        case Sub: return ctx.builder.CreateSub(lhs, rhs, "subtmp");
        case Mul: return ctx.builder.CreateMul(lhs, rhs, "multmp");
        case Div: return ctx.builder.CreateSDiv(lhs, rhs, "divtmp");
    }

    throw std::runtime_error("Invalid ExprOp");
}

// Just generates a constant and returns it
llvm::Value* Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        this->val,
        true
    );
}

// TODO
llvm::Value* Ast::Nodes::ExprLitFloat::generate(Context& ctx) {}; //TODO

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
        llvm::Type::getInt32Ty(ctx.llvmCtx),
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

// Checks if the variable already exists
// If it does, it throws an error and quits
// Otherwise, it creates the variable in the current scope
// If in the global scope, it creates a global variable
llvm::Value* Ast::Nodes::Let::generate(Context& ctx) {
    llvm::Value* initVal = expr->generate(ctx);

    // Making sure that the variable doesn't exist
    auto _var = ctx.lookup(target);
    
    if(_var) {
        throw std::runtime_error("Variable already defined: " + target);
    }

    // GLOBAL SCOPE
    llvm::Function* _fn = *ctx.current_fn;
    if (_fn->getName() == "_start") {
        // TODO: Probably update to have this use the correct var type
        llvm::Type* ty = llvm::Type::getInt32Ty(ctx.llvmCtx);

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

        // Then call an assignment
        ExprAssign(target, std::move(expr)).generate(ctx);

        return var;
    }

    // FUNCTION SCOPE
    llvm::Function* fn = *ctx.current_fn;
    llvm::AllocaInst* alloca =
        ctx.create_entry_block(fn, target,
            llvm::Type::getInt32Ty(ctx.llvmCtx));

    ctx.builder.CreateStore(initVal, alloca);
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

    // Ensure i32
    if (retVal->getType()->isIntegerTy() &&
        retVal->getType()->getIntegerBitWidth() < 32) {
        retVal = ctx.builder.CreateSExt(
            retVal,
            llvm::Type::getInt32Ty(ctx.llvmCtx)
        );
    }

    if(!ctx.module->getFunction("main") && !ctx.module->getFunction("_start")) {
        throw std::runtime_error("Unimplemented return function for non-main function");
    }

    auto asmFn = syscall_exit(ctx.llvmCtx);

    ctx.builder.CreateCall(asmFn, { retVal });
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
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
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

// Creates the function type. TODO: Fix this: (void, no args)
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Pops the scope and resets the current function
// Sets the insert point back to the start block
llvm::Value* Ast::Nodes::Function::generate(Context& ctx) {
    llvm::FunctionType* fn_type =
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(ctx.llvmCtx),
            false
        );

    llvm::Function* new_fn =
        llvm::Function::Create(
            fn_type,
            llvm::Function::ExternalLinkage,
            name,
            *ctx.module
        );

    ctx.current_fn = std::make_shared<llvm::Function*>(new_fn);
    ctx.push_scope();

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(ctx.llvmCtx, "entry", new_fn);
    ctx.builder.SetInsertPoint(entry);

    block->generate(ctx);

    ctx.pop_scope();
    ctx.current_fn.reset();

    ctx.builder.SetInsertPoint(*(ctx._start_block));
    return nullptr;
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& expr : content)
        expr->generate(ctx);
}