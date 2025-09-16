#include <string>

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

    return EXIT_SUCCESS;
}


