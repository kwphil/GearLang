#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>

#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/func.hpp>

using namespace Ast::Nodes;

// Creates the function type. 
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Pops the scope and resets the current function
// Sets the insert point back to the start block
void Function::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(arg->get_type().value().to_llvm(ctx));
    }

    llvm::Function* fn;
    llvm::BasicBlock* entry;
    unsigned idx = 0;

    if(name == "main") {
        fn = ctx.module->getFunction("main");

        entry = llvm::BasicBlock::Create(ctx.llvmCtx, "main_fn", fn);
        ctx.main_entry = std::make_unique<llvm::BasicBlock*>(entry);
    } else {
        fn = declare_func(
            ty.to_llvm(ctx), param_types, name.c_str(), ctx, is_variadic
        );

        entry = llvm::BasicBlock::Create(ctx.llvmCtx, "entry", fn);
    }

    for (auto& llvm_arg : fn->args()) {
        llvm_arg.setName(args[idx++]->name);
    }
    
    ctx.builder.SetInsertPoint(entry);
    ctx.current_fn = fn;
    ctx.push_scope();

    idx = 0;
    for(auto& arg : fn->args()) {
        auto& ast_arg = args[idx];

        // Alloca
        llvm::Type* arg_ty = arg.getType();
        llvm::AllocaInst* alloca = ctx.create_entry_block(
            fn,
            ast_arg->name,
            arg_ty
        );

        ctx.builder.CreateStore(&arg, alloca);

        ast_arg->var = alloca;

        idx++;
    }

    generate_node(block.get(), ctx);

    if (!entry->getTerminator()) {
        ctx.builder.CreateRet(
            llvm::Constant::getNullValue(
                fn->getReturnType()
            )
        );
    }

    ctx.pop_scope();
    ctx.current_fn = ctx.module->getFunction("main");

    ctx.builder.SetInsertPoint(*ctx.global_entry);
}

void Ast::Nodes::ExternFn::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            arg->get_type()->to_llvm(ctx)
        );
    }

    // TODO
    if(no_mangle) {

    }
    
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ty.to_llvm(ctx),
        param_types,
        is_variadic
    );

    llvm::Function::Create(
        fn_type,
        llvm::Function::ExternalLinkage,
        callee,
        *ctx.module
    );
}
