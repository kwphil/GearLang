#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include "ast.hpp"

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
            "main",
            ctx.module
        );

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            mainFn
        );

    ctx.builder.SetInsertPoint(entry);

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

    std::cout << "--- parsed source listing ---\n";
    //root.show(std::cout);
    std::cout << '\n';

    Context ctx;

    llvm::Function* mainFn = create_main(ctx);
    
    std::cout << "generating... ";
    root.generate(ctx);
    std::cout << "done\n";

    ctx.builder.CreateRet(
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
            0
        )
    );

    std::cout << "rendering... ";
    std::string output = ctx.render();

    std::cout << "writing assembler file... ";
    std::ofstream out_file("build.llvm");
    out_file << output;
    out_file.close();
    std::cout << "done\n";

    return EXIT_SUCCESS;
}


