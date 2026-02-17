#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>

#include "../ast/base.hpp"
#include "../ast/stmt.hpp"
#include "../ctx.hpp"
#include "../func.hpp"

// Creates the function type. 
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Pops the scope and resets the current function
// Sets the insert point back to the start block
void Ast::Nodes::Function::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(arg.type.to_llvm(ctx));
    }

    llvm::Function* fn;
    llvm::BasicBlock* entry;
    unsigned idx = 0;

    if(name == "main") {
        fn = ctx.module->getFunction("main");

        entry = llvm::BasicBlock::Create(ctx.llvmCtx, "main_fn", fn);
    } else {
        fn = declare_func(
            ty.to_llvm(ctx), param_types, name.c_str(), ctx, is_variadic
        );

        entry = llvm::BasicBlock::Create(ctx.llvmCtx, "entry", fn);

        for (auto& llvm_arg : fn->args()) {
            llvm_arg.setName(args[idx++].name);
        }
    }

    ctx.current_fn = fn;
    ctx.push_scope();

    idx = 0;
    for(auto& arg : fn->args()) {
        auto& ast_arg = args[idx];

        // Alloca
        llvm::Type* arg_ty = ast_arg.type.to_llvm(ctx);
        llvm::AllocaInst* alloca = ctx.create_entry_block(
            fn,
            ast_arg.name,
            arg_ty
        );

        ctx.builder.CreateStore(&arg, alloca);

        if(ast_arg.type.is_pointer_ty()) {
            Value* val = new Value {
                .ir=alloca,
                .ty=ast_arg.type.get_underlying_type(ctx),
                .is_address=true
            };

            ctx.bind(ast_arg.name, val);
        }

        Value* val = new Value {
            .ir=alloca,
            .ty=arg_ty,
            .is_address=false
        };

        ctx.bind(ast_arg.name, val);

        idx++;
    }

    generate_node(&*block, ctx);

    if (!entry->getTerminator()) {
        ctx.builder.CreateRet(
            llvm::Constant::getNullValue(
                ty.to_llvm(ctx)
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
            arg.type.to_llvm(ctx)
        );
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
