#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <source_location>
#include <memory>

/// @brief Lexer namespace
namespace Lexer {

/// @brief Character types used during lexing
enum class CharType {
    Invalid,
    Alpha,
    Num,
    Paren,
    Sym,
    Format,
    Quote,
    Brace,
    Caret,
    Semi,
    Hash,
    At
};

/// @brief Get the character type of a given character
/// @param c Character to classify
/// @return Character type
CharType getCharType(char c);

/// @brief Token types used in the lexer
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
    Ellipsis,
    Equal,
    Comma,
    Semi,
    Caret,
    Hash,
    At
};

const char* print_type(Type ty);

/// @brief Classify a token based on its content and ending character type
/// @param content Content of the token
/// @param state Ending character type of the token
/// @return Token type
Type classify(std::string& content, CharType state);

/// @brief Token representation
class Token {
public:
    /// @brief Construct a new Token object
    std::string content;
    /// @brief Line number where the token is located
    uint32_t line;
    /// @brief Column number where the token starts
    Type type;
};

/// @brief Stream of tokens produced by the lexer
class Stream {
private:
    /// @brief Current index in the token stream
    uint32_t index = 0;

public:
    /// @brief List of tokens in the stream
    std::vector<Token> content;

    /// @brief Check if there are more tokens in the stream
    bool has();
    /// @brief Peek at the next token without consuming it
    /// @return Pointer to the next token
    std::unique_ptr<Token> peek();
    /// @brief Peek at the token after the next token without consuming anything
    /// @return Pointer to the second next token
    std::unique_ptr<Token> next();
    /// @brief Pop the next token from the stream
    /// @return Pointer to the popped token
    std::unique_ptr<Token> pop();
    /// @brief Move back one token in the stream
    void back() { index--; }
    /// @brief Expect the next token to match a specific content
    /// @param should Expected token content
    /// @param line_number Line expected for the error
    void expect(
        const char* should, 
        int line_number
    );

    /// @brief Dumps the remaining unparsed text
    void dump();
    /// @brief Converts all output into a string, format as { content, line, type }
    std::string to_string();
};

/// @brief Tokenize a source file into a token stream
/// @param source_path Path to the source file
/// @return Token stream
Stream tokenize(std::string& source_path);

} 
