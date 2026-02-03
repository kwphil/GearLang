#include "lex.hpp"
#include "ast.hpp"
#include "error.hpp"

#include <unordered_set>
#include <cctype>
#include <iostream>
#include <fstream>
#include <source_location>
#include <format>

Lexer::CharType Lexer::getCharType(char c) {
    if (isalpha(c) || c == '_') return CharType::Alpha;
    if (isdigit(c) || c == '.') return CharType::Num;
    if (c == '(' || c == ')') return CharType::Paren;
    if (c == '{' || c == '}') return CharType::Brace; 
    if (c == ' ' || c == '\n' || c == '\t') return CharType::Format;
    if (c == '"') return CharType::Quote;
    return CharType::Sym;
}

Lexer::Type Lexer::classify(std::string& content, CharType state)
{
    static const std::unordered_set<std::string> keywords = {
        "fn", "let", "comptime", "assert", "test", "if", "else", "extern"
    };

    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/", "=>", ":"
    };

    switch (state) {
        case CharType::Invalid:
        case CharType::Format:
            break; // unreachable

        case CharType::Alpha:
            return keywords.find(content) == keywords.end()
                ? Type::Identifier
                : Type::Keyword;

        case CharType::Paren:
            if (content == "(") return Type::ParenOpen;
            if (content == ")") return Type::ParenClose;
            break;

        case CharType::Brace:
            if (content == "{") return Type::BraceOpen;
            if (content == "}") return Type::BraceClose;
            break;

        case CharType::Num:
            if (std::any_of(content.begin(), content.end(),
                [](char c){ return std::isalpha(static_cast<unsigned char>(c)); }))
            {
                return Type::Identifier;
            }

            return content.find('.') == std::string::npos
                ? Type::IntegerLiteral
                : Type::FloatLiteral;

        case CharType::Sym:
            if (content == ",") return Type::Comma;
            if (content == "&") return Type::Amper;
            if (operators.find(content) != operators.end())
                return Type::Operator;
            return Type::Invalid;

        case CharType::Quote:
            return Type::StringLiteral;
    }

    throw std::runtime_error(std::format(
        "This should not be reached!!!! Unknown state: {}", 
        (int)state
    ));
    return Type::Invalid;
}

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

#include <format>

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

Lexer::Stream Lexer::tokenize(std::string& source_path)
{
    std::ifstream file(source_path);
    Stream out;

    char c;
    uint32_t line = 1;
    CharType state_new, state_old = CharType::Invalid;
    Token tok{};

    bool is_string = false;
    bool is_comment = false;

    while (file.get(c)) {
        state_new = getCharType(c);

        // state transition -> token boundary
        if ((state_new != state_old) && !is_string) {
            // allow identifiers like "num1"
            if (state_new == CharType::Num &&
                state_old == CharType::Alpha)
                goto transition_cancel;

            if (tok.content == "//") is_comment = true;

            if (state_old != CharType::Invalid &&
                state_old != CharType::Format &&
                (!is_comment || is_string))
            {
                tok.type = classify(tok.content, state_old);
                tok.line = line;
                out.content.push_back(tok);
                tok = Token{};
            } else {
                tok.content.clear();
            }
        }

    transition_cancel:

        if (state_new == CharType::Quote)
            is_string = !is_string;

        if (c == '\n')
        {
            line++;
            is_comment = false;
        }

        tok.content += c;
        state_old = state_new;
    }

    // flush final token if file didn't end with a transition
    if (!tok.content.empty() &&
        state_old != CharType::Format &&
        state_old != CharType::Invalid &&
        (!is_comment || is_string))
    {
        tok.type = classify(tok.content, state_old);
        tok.line = line;
        out.content.push_back(tok);
    }

    return out;
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
