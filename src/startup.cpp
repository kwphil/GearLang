// This file builds the runtime

#include "func.hpp"


llvm::Function* build_runtime(Context& ctx) { 
    // Reworking this to be easier and just call main
    llvm::Type* i32 = llvm::Type::getInt32Ty(ctx.llvmCtx);
    llvm::Type* raw_ptr = llvm::PointerType::get(ctx.llvmCtx, 0);

    llvm::Function* global_fn = declare_func(
        i32, { i32, raw_ptr }, "main", ctx, false
    );

    llvm::BasicBlock* global_fn_entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            global_fn
    );
    
    // Set entry to main
    ctx.builder.SetInsertPoint(global_fn_entry);
    ctx.global_entry = std::make_unique<llvm::BasicBlock*>(global_fn_entry);
    ctx.current_fn = global_fn;

    return global_fn;
}