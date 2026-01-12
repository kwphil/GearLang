#include "lex.hpp"

#include <cctype>
#include <iostream>
#include <fstream>

Lexer::CharType Lexer::getCharType(char c) {
    if (isalpha(c) || c == '_') return CharType::Alpha;
    if (isdigit(c) || c == '.') return CharType::Num;
    if (c == '(' || c == ')') return CharType::Paren;
    if (c == ' ' || c == '\n' || c == '\t') return CharType::Format;
    if (c == '"') return CharType::Quote;
    return CharType::Sym;
}

Lexer::Type Lexer::classify(std::string& content, CharType state)
{
    static const std::unordered_set<std::string> keywords = {
        "fn", "let", "comptime", "assert", "test"
    };

    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/",
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

        case CharType::Num:
            return content.find('.') == std::string::npos
                ? Type::IntegerLiteral
                : Type::FloatLiteral;

        case CharType::Sym:
            if (operators.find(content) != operators.end())
                return Type::Operator;
            return Type::Invalid;

        case CharType::Quote:
            return Type::StringLiteral;
    }

    std::cerr << "This should not be reached!!!!\n";
    return Type::Invalid;
}

bool Lexer::Stream::has() {
    return index != content.size();
}

Lexer::Token Lexer::Stream::peek() {
    return content[index];
}

Lexer::Token Lexer::Stream::pop() {
    return content[index++];
}

void Lexer::Stream::expect(const char* should) {
    auto is = pop().content;
    if (is != should) {
        std::cerr << "Error: Expected '" << should
                  << "', but got '" << is << "'\n";
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

    return out;
}
