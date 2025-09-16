#include <cstdlib>
#include <string>
#include <iostream>

#include "ast.cpp"


int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    std::string source_path(argv[1]);
    auto tokens = Lexer::tokenize(source_path);

    auto root = Ast::Program::parse(tokens);
    std::cout << root.show();

    Context ctx;
    root.generate(ctx);
    
    std::string output = ctx.render();

    std::ofstream out_file("build.asm");
    out_file << output;
    out_file.close();

    system("fasm build.asm build");
    system("chmod +x build");
    system("./build; echo $?");


    return EXIT_SUCCESS;
}


