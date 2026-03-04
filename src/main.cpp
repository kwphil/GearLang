/*
   _____                 _                       
  / ____|               | |                      
 | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
 | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
 | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
  \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
                                            __/ |
                                           |___/ 

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <format>

#include <gearlang/ast/base.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>
#include <gearlang/func.hpp>
#include <gearlang/sem/analyze.hpp>

#include <argparse/argparse.hpp>

#define VERSION "0.1.0"

llvm::Function* build_runtime(Context& ctx);
void init_program(argparse::ArgumentParser& program);
void run_command(const char* cmd, bool verbose);

#define RUN_STEP(note, code) \
    if(verbose) std::cout << note << "..." << std::endl; \
    { code } \
    if(verbose) std::cout << "done\n";                   

int main(int argc, char** argv) {
    argparse::ArgumentParser program("gearlang", VERSION);

    init_program(program);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return EXIT_FAILURE;
    }

    std::string input_file  = program.get<std::string>("input");
    std::string output_file = program.get<std::string>("--output");
    bool verbose            = program.get<bool>("--verbose");
    bool output_object      = program.get<bool>("--object");
    bool output_llvm        = program.get<bool>("--emit-llvm");
    bool dump_tokens        = program.get<bool>("--dump-tokens");
    bool dump_ast           = program.get<bool>("--dump-ast");

    if (input_file.empty()) {
        std::cerr << "No input file provided\n";
        return EXIT_FAILURE;
    }

    Error::setup_error_manager(input_file.c_str());

    Lexer::Stream tokens;
    Ast::Program root;
    
    RUN_STEP("tokenizing",
        tokens = Lexer::tokenize(input_file);
    );

    if(dump_tokens) {
        std::cout << tokens.to_string() << std::endl;
        return EXIT_SUCCESS;
    }

    RUN_STEP("parsing",
        root = Ast::Program::parse(tokens);
    );

    if(dump_ast) {
        std::cout << root.to_string() << std::endl;
        return EXIT_SUCCESS;
    }

    Sem::Analyzer analyzer;

    RUN_STEP("analyzing",
        analyzer.analyze(root.content);
    );

    Context ctx;

    RUN_STEP("generating", {
        ctx.current_fn = build_runtime(ctx);
        root.generate(ctx);

        if(ctx.main_entry) {
            llvm::BasicBlock* main = *ctx.main_entry;
            ctx.builder.CreateBr(main);
            ctx.builder.SetInsertPoint(main);
        }

        ctx.builder.CreateRet(
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(ctx.llvmCtx),
                EXIT_SUCCESS
            )
        );
    });

    std::string output;

    RUN_STEP("rendering", 
        output = ctx.render();    
    );

    run_command("mkdir -p build", verbose);

    std::ofstream out_file("build/build.llvm");
    out_file << output;
    out_file.close();

    run_command("llvm-as build/build.llvm -o build/build.bc", verbose);

    if(output_llvm) {
        run_command("mv build/build.llvm build.llvm", verbose);
        goto cleanup;
    }

    run_command("llc build/build.bc -filetype=obj -o build/build.o", verbose);

    if(output_object) {
        run_command("mv build/build.o build.o", verbose);
        goto cleanup;
    }

    {
        std::string cc =
            std::format("cc build/build.o -o {}", output_file);
        run_command(cc.c_str(), verbose);
    }

    if(verbose) std::cout << "Built successfully!\n";

cleanup:
    if(verbose) std::cout << "Cleanup...\n";
    run_command("rm build/build*", verbose);

    return EXIT_SUCCESS;
}

void run_command(const char* cmd, bool verbose) { 
    if(verbose) std::cout << "> " << cmd << "\n"; 
    if(std::system(cmd)) exit(1); 
}
