#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <format>

#include <gearlang/ast/base.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>
#include <gearlang/func.hpp>
#include <gearlang/etc.hpp>
#include <gearlang/sem/analyze.hpp>

#include <argparse/argparse.hpp>

#define VERSION "0.1.0"

#define RUN_STEP(note, code) \
    if(opts.verbose) std::cout << note << "...\n"; \
    { code } \
    if(opts.verbose) std::cout << "done\n";

llvm::Function* build_runtime(Context& ctx);
void init_program(argparse::ArgumentParser& program);

static Options parse_args(int argc, char** argv);
static std::string compile_ir(const Options& opts);
static void build_output(const Options& opts, const std::string& ir);
static void run_command(const std::string& cmd, bool verbose);
Ast::Program build_tree(const Options& opts);

int main(int argc, char** argv) {
    Options opts = parse_args(argc, argv);

    Error::setup_error_manager(opts.input.c_str());

    std::string ir = compile_ir(opts);

    build_output(opts, ir);

    if(opts.verbose)
        std::cout << "Built successfully!\n";

    return EXIT_SUCCESS;
}

static Options parse_args(int argc, char** argv) {
    argparse::ArgumentParser program("gearlang", VERSION);

    init_program(program);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        exit(EXIT_FAILURE);
    }

    Options opts;

    opts.input       = program.get<std::string>("input");
    opts.output      = program.get<std::string>("--output");
    opts.verbose     = program.get<bool>("--verbose");
    opts.emit_object = program.get<bool>("--object");
    opts.emit_llvm   = program.get<bool>("--emit-llvm");
    opts.dump_tokens = program.get<bool>("--dump-tokens");
    opts.dump_ast    = program.get<bool>("--dump-ast");

    if(opts.input.empty()) {
        std::cerr << "No input file provided\n";
        exit(EXIT_FAILURE);
    }

    return opts;
}

static std::string compile_ir(const Options& opts) {
    Lexer::Stream tokens;
    Ast::Program root = build_tree(opts);
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

    return output;
}

static void build_output(const Options& opts, const std::string& ir) {
    run_command("mkdir -p build", opts.verbose);

    std::ofstream out("build/build.llvm");
    out << ir;
    out.close();

    if(opts.emit_llvm) {
        run_command("mv build/build.llvm build.llvm", opts.verbose);
        return;
    }

    run_command("llvm-as build/build.llvm -o build/build.bc", opts.verbose);
    run_command("llc build/build.bc -filetype=obj -relocation-model=pic -o build/build.o", opts.verbose);

    if(opts.emit_object) {
        run_command("mv build/build.o build.o", opts.verbose);
        return;
    }

    std::string cc =
        std::format("cc build/build.o -fPIE -o {}", opts.output);

    run_command(cc, opts.verbose);

    if(opts.verbose)
        std::cout << "Cleanup...\n";

    run_command("rm build/build*", opts.verbose);
}

static void run_command(const std::string& cmd, bool verbose) {
    if(verbose)
        std::cout << "> " << cmd << "\n";

    if(std::system(cmd.c_str()))
        exit(EXIT_FAILURE);
}
