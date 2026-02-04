#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <llvm/IR/InlineAsm.h>

#include "ast.hpp"
#include "syscall.hpp"
#include "error.hpp"
#include "func.hpp"

llvm::Function* create_main(Context& ctx) { 
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

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    Error::setup_error_manager(argv[1]);
    std::string source_path(argv[1]);
    std::cout << "tokenizing... ";
    auto tokens = Lexer::tokenize(source_path);
    std::cout << "done\n";

    std::cout << "parsing... ";
    auto root = Ast::Program::parse(tokens);
    std::cout << "done\n";

    Context ctx;

    std::cout << "generating... ";

    ctx.current_fn = create_main(ctx);
    root.generate(ctx);

    // Now call main (if it exists)
    auto main = ctx.module->getFunction("main");
    if (main) {
        ctx.builder.CreateCall(ctx.module->getFunction("main"));
    }

    // And return .global_fn
    auto exit_fn = syscall_exit(ctx.llvmCtx);

    auto retVal = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        0,
        true
    );
    ctx.builder.CreateCall(exit_fn, { retVal });
    ctx.builder.CreateUnreachable();

    std::cout << "done\n";

    std::cout << "rendering... ";
    std::string output = ctx.render();

    std::cout << "writing llvm file... ";
    std::ofstream out_file("build.llvm");
    out_file << output;
    out_file.close();
    std::cout << "done\n";

    std::cout << "building... \n";
    std::string command = "llvm-as build.llvm -o build.bc";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(1);
    command = "nasm asm/_start_stub.asm -f elf64 -o _start_stub.o";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(1);
    command = "llc build.bc -filetype=obj -o build.o";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(1);
    command = "cc -nostartfiles build.o _start_stub.o -o build";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(1);
    std::cout << "done\n";

    std::cout << "Built successfully!\n";

    return EXIT_SUCCESS;
}


