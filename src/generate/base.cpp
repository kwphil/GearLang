#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include "../ast.hpp"
#include "../syscall.hpp"

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
    llvm::Function* _fn = ctx.current_fn;
    if (_fn->getName() == "_start") {
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

    if(
        ctx.module->getFunction("main") != ctx.current_fn 
        && ctx.module->getFunction("_start") != ctx.current_fn
    ) {
        ctx.builder.CreateRet(retVal);
        return retVal;
    }

    // Ensure i32
    if (retVal->getType()->isIntegerTy() &&
        retVal->getType()->getIntegerBitWidth() < 32) {
        retVal = ctx.builder.CreateSExt(
            retVal,
            llvm::Type::getInt32Ty(ctx.llvmCtx)
        );
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

#include <iostream>

// Creates the function type. 
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Pops the scope and resets the current function
// Sets the insert point back to the start block
llvm::Value* Ast::Nodes::Function::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            arg.ty == Ast::Type::NonPrimitive
            ? Ast::type_to_llvm_type(arg.npty, ctx)
            : Ast::type_to_llvm_type(arg.ty, ctx)
        );
    }

    // Creating the function type
    llvm::FunctionType* fn_type =
        llvm::FunctionType::get(
            ty == Ast::Type::NonPrimitive
            ? Ast::type_to_llvm_type(npty, ctx)
            : Ast::type_to_llvm_type(ty, ctx),
            param_types,
            false
        );

    llvm::Function* fn =
        llvm::Function::Create(
            fn_type,
            llvm::Function::ExternalLinkage,
            name,
            *ctx.module
        );

    unsigned idx = 0;
    for (auto& llvm_arg : fn->args()) {
        llvm_arg.setName(args[idx++].name);
    }

    ctx.current_fn = fn;
    ctx.push_scope();

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(ctx.llvmCtx, "entry", fn);
    ctx.builder.SetInsertPoint(entry);

    idx = 0;
    for(auto& arg : fn->args()) {
        auto& ast_arg = args[idx];
        ctx.bind(ast_arg.name, &arg);
        idx++;
    }

    block->generate(ctx);

    if (!entry->getTerminator()) {
        if (ty == Ast::Type::Void) {
            ctx.builder.CreateRetVoid();
        } else {
            ctx.builder.CreateRet(
                llvm::Constant::getNullValue(
                    ty != Ast::Type::NonPrimitive 
                    ? Ast::type_to_llvm_type(ty, ctx)
                    : Ast::type_to_llvm_type(npty, ctx)
                )
            );
        }
    }

    ctx.pop_scope();
    ctx.current_fn = ctx.module->getFunction("_start");
    ctx.builder.SetInsertPoint(*ctx._start_block);

    return fn;
}

llvm::Value* Ast::Nodes::ExternFn::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            arg.ty == Ast::Type::NonPrimitive
            ? Ast::type_to_llvm_type(arg.npty, ctx)
            : Ast::type_to_llvm_type(arg.ty, ctx)
        );
    }
    
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ty == Ast::Type::NonPrimitive
            ? Ast::type_to_llvm_type(npty, ctx)
            : Ast::type_to_llvm_type(ty, ctx),
        param_types,
        false
    );

    return llvm::Function::Create(
        fn_type,
        llvm::Function::ExternalLinkage,
        callee,
        *ctx.module
    );
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& expr : content)
        expr->generate(ctx);
}
