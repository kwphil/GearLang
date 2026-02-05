// This file builds the runtime

#include "func.hpp"


llvm::Function* build_runtime(Context& ctx) { 
    auto _void = llvm::Type::getVoidTy(ctx.llvmCtx);
    auto i32 = llvm::Type::getInt32Ty(ctx.llvmCtx);
    auto i64 = llvm::Type::getInt64Ty(ctx.llvmCtx);
    auto raw_ptr = llvm::PointerType::get(ctx.llvmCtx, 0);
    
    llvm::Function* _start_fn = declare_func(
        _void, { raw_ptr } , "_entry", ctx, false
    ); 

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            _start_fn
        );

    ctx.builder.SetInsertPoint(entry);

    // Call glibc setup

    // declare i32 @__libc_start_main(
    //    ptr, i64, ptr, ptr, ptr, ptr, ptr
    // )

    // Declaring the function
    llvm::Function* libc_fn = declare_func(
        i32,
        { 
            raw_ptr, i64, raw_ptr, raw_ptr,
            raw_ptr, raw_ptr, raw_ptr 
        },
        "__libc_start_main", ctx, false
    );

    // define i32 @.global_fn(i32 %argc, ptr %argv, ptr %envp) {
    // entry:
    //     call void @do_all_the_things()
    //     call void @exit(i32 0)
    //     unreachable
    // }

    llvm::Function* global_fn = declare_func(
        i32, { i32, raw_ptr }, ".global_fn", ctx, false
    );

    llvm::BasicBlock* global_fn_entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            global_fn
        );
    
    // nullptr
    auto nulptr = llvm::Constant::getNullValue(raw_ptr);
    // char
    llvm::Type* i8 = llvm::Type::getInt8Ty(ctx.llvmCtx);
    // char* array
    llvm::ArrayType* arr_i8 = 
        llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx.llvmCtx), 1);
    // NULL char
    llvm::Constant* i8_null = llvm::Constant::getNullValue(i8);
    
    // argv
    auto min_argv = llvm::ConstantArray::get(
        arr_i8,
        { i8_null }
    );

    llvm::Constant* i32_null = 
        llvm::Constant::getNullValue(i32);

    // Call __libc_start_main
    ctx.builder.CreateCall(
        libc_fn->getFunctionType(),
        libc_fn, 
        { global_fn, i32_null, min_argv, nulptr,
          nulptr, nulptr, _start_fn->getArg(0) }
    );

    // From here will be unreachable
    ctx.builder.CreateUnreachable();
    
    // Set entry to .global_fn
    ctx.builder.SetInsertPoint(global_fn_entry);
    ctx.global_entry = std::make_unique<llvm::BasicBlock*>(global_fn_entry);
    ctx.current_fn = global_fn;

    return global_fn;
}