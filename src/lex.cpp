#include "lex.hpp"
#include "ast.hpp"

#include <cctype>
#include <iostream>
#include <fstream>
#include <source_location>

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
        "fn", "let", "comptime", "assert", "test"
    };

    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/", "=>"
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

std::unique_ptr<Lexer::Token> Lexer::Stream::peek() {
    if(index >= content.size()) return nullptr;
    return std::make_unique<Lexer::Token>(content[index]);
}

std::unique_ptr<Lexer::Token> Lexer::Stream::pop() {
    return std::make_unique<Lexer::Token>(content[index++]);
}

void Lexer::Stream::expect(
    const char* should,
    const std::source_location& location
) {
    auto is = pop()->content;
    if (is != should) {
        std::cerr << 
            "Parser found an error at: " << location.file_name() << ":"
            << location.line() << ": " << location.function_name() << "\n"
            "Error: Expected '" << should << "', but got '" << is << "'\n";

        // exit(EXIT_FAILURE);
    }
}

void Lexer::Stream::expect(
    const char* should,
    std::unique_ptr<Ast::Nodes::NodeBase>& nodes_parsed,
    const std::source_location& location
) {
    auto is = pop()->content;
    if (is != should) {
        std::cerr << 
            "Parser found an error at: " << location.file_name() << ":"
            << location.line() << ": " << location.function_name() << "\n"
            "Error: Expected '" << should << "', but got '" << is << "'.\n"
            "Parser threw an error on line: " << nodes_parsed->line_number << "\n";

        // exit(EXIT_FAILURE);
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
