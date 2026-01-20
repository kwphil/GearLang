#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <source_location>
#include <memory>

// stupid circular dependencies
namespace Ast::Nodes {
    class NodeBase;
}

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
    Brace,
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
    BraceOpen,
    BraceClose,
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
    void back() { index--; }
    void expect(
        const char* should, 
        const std::source_location& location = std::source_location::current()
    );

    void expect(
        const char* should,
        std::unique_ptr<Ast::Nodes::NodeBase>& nodes_parsed,
        const std::source_location& location = std::source_location::current()
    );
};

Stream tokenize(std::string& source_path);

} 