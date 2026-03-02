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

void run_command(const char* cmd, bool verbose);

#define RUN_STEP(note, code) \
    if(verbose) std::cout << note << "..." << std::endl; \
    { code } \
    if(verbose) std::cout << "done\n";                   

int main(int argc, char** argv) {
    argparse::ArgumentParser program("gearlang", VERSION);

    program.add_description("Gearlang compiler");

    program.add_argument("input")
        .help("Input source file");

    program.add_argument("-o", "--output")
        .help("Output executable file")
        .default_value(std::string("a.out"));

    program.add_argument("-G", "--object")
        .help("Emit object file")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-S", "--emit-llvm")
        .help("Emit LLVM IR file")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-v", "--verbose")
        .help("Enable verbose output")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--version")
        .help("Print version information")
        .default_value(false)
        .implicit_value(true);
    
    program.add_argument("--dump-tokens")
        .help("Print token output")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return EXIT_FAILURE;
    }

    if (program.get<bool>("--version")) {
        std::cout << VERSION << "\n";
        return EXIT_SUCCESS;
    }

    std::string input_file  = program.get<std::string>("input");
    std::string output_file = program.get<std::string>("--output");
    bool verbose            = program.get<bool>("--verbose");
    bool output_object      = program.get<bool>("--object");
    bool output_llvm        = program.get<bool>("--emit-llvm");
    bool dump_tokens        = program.get<bool>("--dump-tokens");

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