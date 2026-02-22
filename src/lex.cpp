#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

#include <iostream>
#include <fstream>
#include <format>

Lexer::CharType Lexer::getCharType(char c);

bool Lexer::Stream::has() {
    return index != content.size();
}

std::unique_ptr<Lexer::Token> Lexer::Stream::peek() {
    if(index >= content.size()) return nullptr;
    return std::make_unique<Lexer::Token>(content[index]);
}

std::unique_ptr<Lexer::Token> Lexer::Stream::next() {
    if(index + 1 >= content.size()) return nullptr;
    return std::make_unique<Lexer::Token>(content[index+1]);
}

std::unique_ptr<Lexer::Token> Lexer::Stream::pop() {
    return std::make_unique<Lexer::Token>(content[index++]);
}

void Lexer::Stream::expect(
    const char* should,
    int line_number
) {
    auto is = pop()->content;
    if (is != should) {
        Error::throw_error(
            line_number,
            is.c_str(),
            std::format(
                "Parser found an error. Expected `{}` but received `{}`",
                should, is
            ).c_str(),
            Error::ErrorCodes::EXPECT_VALUE
        );
    }
}

void Lexer::Stream::dump() {
    for (size_t i = index; i < content.size(); i++)
        std::cout << content[i].content << " ";
    std::cout << "\n";
}

std::string Lexer::print_type(Lexer::Type ty) {
    using namespace Lexer; // Just to clean a little

    switch(ty) {
        case(Type::Invalid): return "Invalid";
        case(Type::BraceClose): return "BraceClose";
        case(Type::BraceOpen): return "BraceOpen";
        case(Type::ParenClose): return "ParenClose";
        case(Type::ParenOpen): return "ParenOpen";
        case(Type::FloatLiteral): return "FloatLiteral";
        case(Type::IntegerLiteral): return "IntegerLiteral";
        case(Type::StringLiteral): return "StringLiteral";
        case(Type::Keyword): return "Keyword";
        case(Type::Identifier): return "Identifier";
        default: 
            return std::format("Token type: {} is not printable", (int)ty);
    }
}
