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

std::string Lexer::Stream::to_string() {
    std::string out;
    
    for(auto& tok : content) {
        std::string token_type = print_type(tok.type);
        // Making sure escape characters get rendered as regular text
        std::string content;
        for(auto& c : tok.content) {
            if(c == '\n') {
                content.push_back('\\');
                content.push_back('n');
                continue;
            }

            if(c == '\t') {
                content.push_back('\\');
                content.push_back('t');
                continue;
            }

            content.push_back(c);
        }

        std::string curr = std::format("{{{}, {}, {}}}\n", 
            content, 
            tok.line, 
            token_type
        );

        out += curr;
    }

    return out;
}

const char* Lexer::print_type(Lexer::Type ty) {
    using namespace Lexer; // Just to clean a little

    switch(ty) {
        case(Type::BraceClose): return "BraceClose";
        case(Type::BraceOpen): return "BraceOpen";
        case(Type::ParenClose): return "ParenClose";
        case(Type::ParenOpen): return "ParenOpen";
        case(Type::FloatLiteral): return "FloatLiteral";
        case(Type::IntegerLiteral): return "IntegerLiteral";
        case(Type::StringLiteral): return "StringLiteral";
        case(Type::Keyword): return "Keyword";
        case(Type::Identifier): return "Identifier";
        case(Type::Operator): return "Operator";
        case(Type::Ellipsis): return "Ellipsis";
        case(Type::Comma): return "Comma";
        case(Type::Semi): return "Semi";
        case(Type::Caret): return "Caret";
        case(Type::Hash): return "Hash";
        case(Type::Equal): return "Equal";
        case(Type::At): return "At";
        default: return "Invalid";
    }
}