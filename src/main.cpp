#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <format>

#include <llvm/IR/InlineAsm.h>

#include <gearlang/ast/base.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>
#include <gearlang/func.hpp>
#include <gearlang/sem/analyze.hpp>

#define VERSION "0.1.0"

llvm::Function* build_runtime(Context& ctx);

void run_command(const char* cmd, bool verbose);

typedef struct {
    int argc;
    char** argv;
    int index;
    char* input_file;
    const char* output_file;
    bool output_object;
    bool output_llvm;
    bool verbose;
} compopt_t;

void compopt_setup(compopt_t** compopt, int argc, char** argv) {
    *compopt = new compopt_t {
        argc, argv, 0, nullptr, "a.out", false, false, false
    };
}

void parse_output(compopt_t* compopt);

void match_flags(compopt_t* compopt);

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    compopt_t* compopt;

    compopt_setup(&compopt, argc, argv);
    
    parse_output(compopt);

    if(!compopt->input_file) {
        std::cerr << "No input file provided\n";
        exit(-1);
    }

    Error::setup_error_manager(argv[1]);
    std::string source_path(argv[1]);
    if(compopt->verbose) std::cout << "tokenizing... ";
    auto tokens = Lexer::tokenize(source_path);
    if(compopt->verbose) std::cout << "done\n";

    if(compopt->verbose) std::cout << "parsing... ";
    auto root = Ast::Program::parse(tokens);
    if(compopt->verbose) std::cout << "done\n";

    Sem::Analyzer analyzer;
    analyzer.analyze(root.content);

    return 0;
    Context ctx;

    if(compopt->verbose) std::cout << "generating... ";

    ctx.current_fn = build_runtime(ctx);
    root.generate(ctx);

    // Return main
    // Set to main_fn if it exists
    // Otherwise the global_entry already exists
    if(ctx.main_entry) {
        llvm::BasicBlock* main = *ctx.main_entry;

        // While we're here, jump to the main_entry rq
        ctx.builder.CreateBr(main);

        ctx.builder.SetInsertPoint(main);
    }
    
    ctx.builder.CreateRet(
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx), 
            EXIT_SUCCESS
        )
    );
    
    ctx.builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx.llvmCtx), EXIT_SUCCESS));

    if(compopt->verbose) std::cout << "done\n";

    if(compopt->verbose) std::cout << "rendering...";
    std::string output = ctx.render();
    if(compopt->verbose) std::cout << "done\n";

    if(compopt->verbose) std::cout << "writing llvm file...\n";
    run_command("mkdir -p build", compopt->verbose);
    std::ofstream out_file("build/build.llvm");
    out_file << output;
    out_file.close();
    if(compopt->verbose) std::cout << "done\n";

    if(compopt->verbose) std::cout << "building... \n";
    run_command("llvm-as build/build.llvm -o build/build.bc", compopt->verbose);
    if(compopt->output_llvm) { run_command("mv build/build.llvm build.llvm", compopt->verbose); goto cleanup; }
    run_command("llc build/build.bc -filetype=obj -o build/build.o", compopt->verbose);
    if(compopt->output_object) { run_command("mv build/build.o build.o", compopt->verbose); goto cleanup; }
    { // Force cc out of scope so goto cleanup works
        std::string cc = 
            std::format(
                "cc build/build.o -o {}", 
                compopt->output_file
            );
        run_command(cc.c_str(), compopt->verbose);
    }

    if(compopt->verbose) std::cout << "Built successfully!\n";

cleanup:
    if(compopt->verbose) std::cout << "Cleanup...\n";
    run_command("rm build/build*", compopt->verbose);

    return EXIT_SUCCESS;
}

void parse_output(compopt_t* compopt) {
    int i = 1;
    for(; i < compopt->argc; i++) {
        char* curr_tok = compopt->argv[i];

        if(curr_tok[0] != '-') { // no - indicates no flags or options (aka file)
            compopt->input_file = curr_tok;
        }

        if(strcmp(curr_tok, "--verbose") == 0) {
            compopt->verbose = true;
            continue;
        }

        if(strcmp(curr_tok, "--version") == 0) {
            std::cout << VERSION << "\n";

            exit(EXIT_SUCCESS);
        }

        if(strcmp(curr_tok, "--object") == 0) {
            compopt->output_object = true;
            continue;
        }

        if(strcmp(curr_tok, "--output") == 0) {
            i++;
            compopt->output_file = curr_tok;
            continue;
        }

        if(strcmp(curr_tok, "--emit-llvm") == 0) {
            compopt->output_llvm = true;
            continue;
        }

        if(curr_tok[0] == '-' && curr_tok[1] != '-') {
            bool set_output_file = false;
            compopt->index = i;

            match_flags(compopt);

            if(set_output_file) {
                i++;
                compopt->output_file = compopt->argv[i];
            }
        }
    }

    compopt->index = i;
}

void run_command(const char* cmd, bool verbose) {
    if(verbose) std::cout << "> " << cmd << "\n";
    if(std::system(cmd)) exit(1);
}

void match_flags(compopt_t* compopt) {
    // Skip the dash
    char* curr_tok = compopt->argv[compopt->index];
    for(int i = 1; i < strlen(curr_tok); i++) {
        switch(curr_tok[i]) {
            case('G'): compopt->output_object = true; break;
            case('S'): compopt->output_llvm = true; break;
            case('v'): compopt->verbose = true; break;
            case('o'): 
                compopt->index++;
                compopt->output_file = compopt->argv[compopt->index];
            break;
            default: std::cerr << "Unknown option: " << curr_tok[i] << '\n'; exit(1);
        }
    }
}
