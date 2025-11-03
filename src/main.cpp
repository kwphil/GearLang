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
    std::cout << "tokenizing... ";
    auto tokens = Lexer::tokenize(source_path);
    std::cout << "done\n";

    std::cout << "parsing... ";
    auto root = Ast::Program::parse(tokens);
    std::cout << "done\n";

    std::cout << "--- parsed source listing ---\n";
    std::cout << root.show();
    std::cout << '\n';

    Context ctx;

    std::cout << "generating... ";
    root.generate(ctx);
    std::cout << "done\n";
    
    std::cout << "rendering... ";
    std::string output = ctx.render();
    std::cout << "done\n";

    std::cout << "writing assembler file... ";
    std::ofstream out_file("build.asm");
    out_file << output;
    out_file.close();
    std::cout << "done\n";

    std::cout << "assembling... ";
    system("fasm build.asm build");
    system("chmod +x build");
    std::cout << "done\n";

    std::cout << "--- running ---\n";
    system("./build; echo $?");


    return EXIT_SUCCESS;
}


