#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <llvm/IR/InlineAsm.h>

#include "ast.hpp"

std::shared_ptr<llvm::Function*> create_main(Context& ctx) {
    llvm::FunctionType* mainType =
        llvm::FunctionType::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
            false
        );

    llvm::Function* mainFn =
        llvm::Function::Create(
            mainType,
            llvm::Function::ExternalLinkage,
            "_start",
            &*(ctx.module)
        );

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(
            ctx.llvmCtx,
            "entry",
            mainFn
        );

    ctx.builder.SetInsertPoint(entry);

    return std::make_shared<llvm::Function*>(mainFn);
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
    root.show(std::cout);
    std::cout << '\n';

    Context ctx;

    ctx.mainFn = create_main(ctx);
    
    std::cout << "generating... ";
    root.generate(ctx);
    std::cout << "done\n";

    ctx.builder.CreateUnreachable();

    std::cout << "rendering... ";
    std::string output = ctx.render();

    std::cout << "writing llvm file... ";
    std::ofstream out_file("build.llvm");
    out_file << output;
    out_file.close();
    std::cout << "done\n";

    std::cout << "compiling llvm file... \n";
    std::string command = "llvm-as build.llvm -o build.bc";
    std::cout << "> " << command << "\n";
    std::system(command.c_str());
    command = "llc build.bc -filetype=obj -o build.o";
    std::cout << "> " << command << "\n";
    std::system(command.c_str());
    command = "ld build.o -o build";
    std::cout << "> " << command << "\n";
    std::system(command.c_str());
    std::cout << "done\n";

    std::cout << "running...";
    command = "./build";
    std::system(command.c_str());
    std::cout << "done; exit=" << std::system("echo $?") << "\n";

    return EXIT_SUCCESS;
}


