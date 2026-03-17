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

#include <gearlang/func.hpp>
#include <gearlang/ast/base.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/etc.hpp>

#include <argparse/argparse.hpp>

#define RUN_STEP(note, code) \
    if(opts.verbose) std::cout << note << "...\n"; \
    { code } \
    if(opts.verbose) std::cout << "done\n";

void init_program(argparse::ArgumentParser& program) {
    program.add_description("Gearlang compiler");

    program.add_argument("input")
        .help("Input source file");

    program.add_argument("-o", "--output")
        .help("Output executable file")
        .default_value(std::string("a.out"));

    program.add_argument("-c", "--object")
        .help("Emit object file")
        .flag();

    program.add_argument("-S", "--emit-llvm")
        .help("Emit LLVM IR file")
        .flag();

    program.add_argument("-v", "--verbose")
        .help("Enable verbose output")
        .flag();
    
    program.add_argument("--dump-tokens")
        .help("Print token output. Used for debugging the compiler")
        .flag();

    program.add_argument("--dump-ast")
        .help("Print the AST output. Used for debugging the compiler")
        .flag();

}

llvm::Function* build_runtime(Context& ctx) { 
    // Reworking this to be easier and just call main
    llvm::Type* i32 = llvm::Type::getInt32Ty(ctx.llvmCtx);
    llvm::Type* raw_ptr = llvm::PointerType::get(ctx.llvmCtx, 0);

    llvm::Function* global_fn = declare_func(
        i32, { i32, raw_ptr }, "main", ctx, false, true
    );

    global_fn->getArg(0)->setName("argc");
    global_fn->getArg(1)->setName("argv");

    llvm::BasicBlock* global_fn_entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            global_fn
    );
    
    // Set entry to main
    ctx.builder.SetInsertPoint(global_fn_entry);
    ctx.global_entry = std::make_unique<llvm::BasicBlock*>(global_fn_entry);
    ctx.main_entry = nullptr;
    ctx.current_fn = global_fn;

    return global_fn;
}

#include <gearlang/ffi/c.hpp>

Ast::Program build_tree(const Options& opts) {
    Lexer::Stream tokens;
    Ast::Program root;

    RUN_STEP("tokenizing",
        tokens = Lexer::tokenize(opts.input);
    );

    if(opts.dump_tokens) {
        std::cout << tokens.to_string() << "\n";
        exit(EXIT_SUCCESS);
    }

    RUN_STEP("parsing",
        root = Ast::Program::parse(tokens);
    );

    if(opts.dump_ast) {
        std::cout << root.to_string() << "\n";
        exit(EXIT_SUCCESS);
    }

    Sem::Analyzer analyzer;

    C_Ffi ffi_test;
    ffi_test.add_header("sys/stdio.h");

    RUN_STEP("C file parsing", {
        root.add_nodes(ffi_test.compile_headers());
    })

    RUN_STEP("analyzing",
        analyzer.analyze(root.content);
    );

    return root;
}
