#include <string>

#include "lex.cpp"


int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Argument not provided correctly.";
        return EXIT_FAILURE;
    }

    std::string source_path(argv[1]);
    Lexer::Stream tokens = Lexer::tokenize(source_path);

    for(Lexer::Token t : tokens.content)
        std::cout << t.content << '\n';
    

    return EXIT_SUCCESS;
}


