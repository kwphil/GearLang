/*
   _____                 _                       
  / ____|               | |                      
 | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
 | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
 | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
  \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
                                            __/ |
                                           |___/ 

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <source_location>
#include <memory>

/// @brief Lexer namespace
namespace Lexer {

/// @brief span metadata for tokens
struct Span {
    size_t line;
    size_t col;
    size_t len;
};

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
    /// @brief The span metadata of the token
    Span span;
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
