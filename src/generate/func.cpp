#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>

#include "../ctx.hpp"
#include "../ast.hpp"

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
        param_types.push_back(arg.ty.generate(ctx));
    }

    // Creating the function type
    llvm::FunctionType* fn_type =
        llvm::FunctionType::get(
            ty.generate(ctx),
            param_types,
            is_variadic
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
        ctx.builder.CreateRet(
            llvm::Constant::getNullValue(
                ty.generate(ctx)
            )
        );
    }

    ctx.pop_scope();
    ctx.current_fn = ctx.module->getFunction(".global_fn");

    ctx.builder.SetInsertPoint(*ctx.global_entry);

    return fn;
}

llvm::Value* Ast::Nodes::ExternFn::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            arg.ty.generate(ctx)
        );
    }
    
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ty.generate(ctx),
        param_types,
        is_variadic
    );

    return llvm::Function::Create(
        fn_type,
        llvm::Function::ExternalLinkage,
        callee,
        *ctx.module
    );
}