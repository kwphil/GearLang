#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <llvm/IR/InlineAsm.h>

#include "ast.hpp"
#include "syscall.hpp"

llvm::Function* create_main(Context& ctx) {
    llvm::FunctionType* mainType =
        llvm::FunctionType::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
            false
        );

    llvm::Function* mainFn =
        llvm::Function::Create(
            mainType,
            llvm::Function::ExternalLinkage,
            "_start",
            ctx.module.get()
        );

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            mainFn
        );

    ctx.builder.SetInsertPoint(entry);
    ctx._start_block = std::make_unique<llvm::BasicBlock*>(entry);

    return mainFn;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

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

    // And return _start
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

    std::cout << "compiling llvm file... \n";
    std::string command = "llvm-as build.llvm -o build.bc";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(2);
    command = "llc build.bc -filetype=obj -o build.o";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(3);
    command = "cc -nostartfiles build.o -o build";
    std::cout << "> " << command << "\n";
    if(std::system(command.c_str())) exit(4);
    std::cout << "done\n";

    std::cout << "Built successfully!\n";

    return EXIT_SUCCESS;
}


