#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <llvm/IR/InlineAsm.h>

#include "ast/base.hpp"
#include "lex.hpp"
#include "syscall.hpp"
#include "error.hpp"

#define VERSION "0.1.0"

llvm::Function* build_runtime(Context& ctx);

void run_command(const char* cmd, bool verbose);

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    const char* file_input;
    bool verbose = false;

    for(int i = 1; i < argc; i++) {
        const char* curr_tok = argv[i];

        if(curr_tok[0] != '-') { // no - indicates no flags or options (aka file)
            file_input = curr_tok;
        }

        if(strcmp(curr_tok, "--verbose") == 0) {
            verbose = true;
        }

        if(strcmp(curr_tok, "--version") == 0) {
            std::cout << VERSION << "\n";

            exit(EXIT_SUCCESS);
        }
    } 

    if(!file_input) {
        std::cerr << "No input file provided\n";
        exit(-1);
    }

    Error::setup_error_manager(argv[1]);
    std::string source_path(argv[1]);
    if(verbose) std::cout << "tokenizing... ";
    auto tokens = Lexer::tokenize(source_path);
    if(verbose) std::cout << "done\n";

    if(verbose) std::cout << "parsing... ";
    auto root = Ast::Program::parse(tokens);
    if(verbose) std::cout << "done\n";

    Context ctx;

    if(verbose) std::cout << "generating... ";

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

    if(verbose) std::cout << "done\n";

    if(verbose) std::cout << "rendering...";
    std::string output = ctx.render();
    if(verbose) std::cout << "done\n";

    if(verbose) std::cout << "writing llvm file...\n";
    run_command("mkdir -p build", verbose);
    std::ofstream out_file("build/build.llvm");
    out_file << output;
    out_file.close();
    if(verbose) std::cout << "done\n";

    if(verbose) std::cout << "building... \n";
    run_command("llvm-as build/build.llvm -o build/build.bc", verbose);
    run_command("nasm asm/_start_stub.asm -f elf64 -o build/_start_stub.o", verbose);
    run_command("llc build/build.bc -filetype=obj -o build/build.o", verbose);
    run_command("cc -nostartfiles build/build.o build/_start_stub.o -o build/build", verbose);

    if(verbose) std::cout << "Built successfully!\n";

    return EXIT_SUCCESS;
}

void run_command(const char* cmd, bool verbose) {
    if(verbose) std::cout << "> " << cmd << "\n";
    if(std::system(cmd)) exit(1);
}
