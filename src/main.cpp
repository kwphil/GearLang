#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <format>

#include <llvm/IR/InlineAsm.h>

#include "ast/base.hpp"
#include "lex.hpp"
#include "error.hpp"
#include "func.hpp"

#define VERSION "0.1.0"

llvm::Function* build_runtime(Context& ctx);

void run_command(const char* cmd, bool verbose);
void match_flags(
    const char* curr_tok, 
    bool* output_object, 
    bool* output_llvm,
    bool* set_output_file
);

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    const char* file_input;
    std::string output_file = "a.out";
    bool verbose = false;
    bool output_object = false;
    bool output_llvm = false;

    for(int i = 1; i < argc; i++) {
        char* curr_tok = argv[i];

        if(curr_tok[0] != '-') { // no - indicates no flags or options (aka file)
            file_input = curr_tok;
        }

        if(strcmp(curr_tok, "--verbose") == 0) {
            verbose = true;
            continue;
        }

        if(strcmp(curr_tok, "--version") == 0) {
            std::cout << VERSION << "\n";

            exit(EXIT_SUCCESS);
        }

        if(strcmp(curr_tok, "--object") == 0) {
            output_object = true;
            continue;
        }

        if(strcmp(curr_tok, "--output") == 0) {
            i++;
            output_file = curr_tok;
            continue;
        }

        if(strcmp(curr_tok, "--emit-llvm") == 0) {
            output_llvm = true;
            continue;
        }

        if(curr_tok[0] == '-' && curr_tok[1] != '-') {
            bool set_output_file = false;

            match_flags(curr_tok, &output_object, &output_llvm, &set_output_file);

            if(set_output_file) {
                i++;
                output_file = argv[i];
            }
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
    call_exit(ctx, new Value { 
        llvm::ConstantInt::get( llvm::Type::getInt32Ty(ctx.llvmCtx), 0 ), 
        llvm::Type::getInt32Ty(ctx.llvmCtx), false 
    });

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
    if(output_llvm) { run_command("mv build/build.llvm build.llvm", verbose); goto cleanup; }
    run_command("nasm asm/_start_stub.asm -f elf64 -o build/_start_stub.o", verbose);
    run_command("llc build/build.bc -filetype=obj -o build/build.o", verbose);
    if(output_object) { run_command("mv build/build.o build.o", verbose); goto cleanup; }
    { // Force cc out of scope so goto cleanup works
        std::string cc = 
            std::format(
                "cc -nostartfiles build/build.o build/_start_stub.o -o {}", 
                output_file
            );
        run_command(cc.c_str(), verbose);
    }

    if(verbose) std::cout << "Built successfully!\n";

cleanup:
    if(verbose) std::cout << "Cleanup...\n";
    run_command("rm -r build", verbose);

    return EXIT_SUCCESS;
}

void run_command(const char* cmd, bool verbose) {
    if(verbose) std::cout << "> " << cmd << "\n";
    if(std::system(cmd)) exit(1);
}

void match_flags(const char* curr_tok, bool* output_object, bool* output_llvm, bool* set_output_file) {
    // Skip the dash
    for(int i = 1; i < strlen(curr_tok); i++) {
        switch(curr_tok[i]) {
            case('G'): *output_object = true; break;
            case('S'): *output_llvm = true; break;
            case('o'): *set_output_file = true; break;
            default: std::cerr << "Unknown option: " << curr_tok[i] << '\n'; exit(1);
        }
    }
}