#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace Lexer {

// finite state machine
enum class CharType {
    Invalid,
    Alpha,
    Num,
    Paren,
    Sym,
    Format,
    Quote,
};

CharType getCharType(char c);

enum class Type {
    Invalid,

    Keyword,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    Operator,
    ParenOpen,
    ParenClose,
};

Type classify(std::string& content, CharType state);

class Token {
public:
    std::string content;
    uint32_t line;
    Type type;
};

class Stream {
private:
    uint32_t index = 0;

public:
    std::vector<Token> content;

    bool has();
    Token peek();
    Token pop();
    void expect(const char* should);
};

Stream tokenize(std::string& source_path);

} 