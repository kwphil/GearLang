#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <llvm/IR/InlineAsm.h>

#include "ast/base.hpp"
#include "lex.hpp"
#include "syscall.hpp"
#include "error.hpp"
#include "func.hpp"

llvm::Function* build_runtime(Context& ctx);

void run_command(const char* cmd);

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

    ctx.current_fn = build_runtime(ctx);
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

    std::cout << "rendering...";
    std::string output = ctx.render();
    std::cout << "done\n";

    std::cout << "writing llvm file...\n";
    run_command("mkdir -p build");
    std::ofstream out_file("build/build.llvm");
    out_file << output;
    out_file.close();
    std::cout << "done\n";

    std::cout << "building... \n";
    run_command("llvm-as build/build.llvm -o build/build.bc");
    run_command("nasm asm/_start_stub.asm -f elf64 -o build/_start_stub.o");
    run_command("llc build/build.bc -filetype=obj -o build/build.o");
    run_command("cc -nostartfiles build/build.o build/_start_stub.o -o build/build");

    std::cout << "Built successfully!\n";

    return EXIT_SUCCESS;
}

void run_command(const char* cmd) {
    std::cout << "> " << cmd << "\n";
    if(std::system(cmd)) exit(1);
}