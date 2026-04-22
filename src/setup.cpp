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

    program.add_argument("--dump-analyzer")
        .help("Print the analyzer output. Used for debugging the compiler")
        .flag();

    program.add_argument("-O", "--opt-level")
        .help("How aggressive the compiler is (e.g. -O 0, -O 1, ...)")
        .default_value(1)
        .scan<'i', int>();
    
    program.add_argument("--no-color")
        .help("Disables color output on diagnostics")
        .flag();
}

#include <gearlang/ffi/c.hpp>
#include <gearlang/optimizer.hpp>

std::unordered_map<std::string, std::unique_ptr<Ffi>> ffi_list;
int Optimizer::opts = 0;

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

    Error::flush();

    if(opts.dump_ast) {
        std::cout << root.to_string() << "\n";
        exit(EXIT_SUCCESS);
    }

    Sem::Analyzer analyzer(opts.dump_analyzer);

    RUN_STEP("Ffi management", {
        for(auto& curr : ffi_list) {
            auto new_nodes = curr.second->compile_headers();
            
            if(opts.verbose) for(auto& t : new_nodes) {
                std::cout << t->to_string() << std::endl;
            }

            root.add_nodes(std::move(new_nodes));
        }
    });

    if(opts.verbose) Type::dump_alias();

    if(opts.opt_level == 1) {
        Optimizer::opts = DEAD_CODE_ELIMINATION | OPERATION_FOLDING;
    }

    // Fill in the rest of the types
    Type::parse_unparsed();

    RUN_STEP("analyzing",
        analyzer.analyze(root.content);
    );

    Error::flush();

    analyzer.dump();

    return root;
}
