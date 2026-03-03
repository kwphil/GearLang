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

#include <string>
#include <fstream>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include <format>

#include <gearlang/lex.hpp>

bool is_single_char_token(Lexer::CharType t);

char get_escape(char c) {
    switch(c) {
    case('n'):
        return '\n';
    case('t'):
        return '\t';
    default: return c;
    }
}

Lexer::Stream Lexer::tokenize(std::string& source_path)
{
    std::ifstream file(source_path);
    Stream out;

    char c;
    size_t line = 1;
    size_t col = 0;
    size_t len = 0;
    CharType state_new, state_old = CharType::Invalid;
    Token tok{};

    bool is_string = false;
    bool is_comment = false;

    while (file.get(c)) {
        len++;
        col++;

        if(c == '\\') {
            file.get(c);
            tok.content.push_back(get_escape(c));
            continue;
        } 
        
        state_new = getCharType(c);
        // state transition -> token boundary
        // Forces a single char token to stay that way
        if ((!is_string) &&
            (state_new != state_old ||
             is_single_char_token(state_new) ||
             is_single_char_token(state_old)))
        {
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
                tok.span = 
                tok.span = { .line = line, .col = col, .len = len };
                out.content.push_back(tok);
                tok = Token{};
                len = 0;
            } else {
                tok.content.clear();
                len = 0;
            }
        }

    transition_cancel:

        if (state_new == CharType::Quote) {
            if(is_string) {
                tok.content = tok.content.substr(1); // Removing the quote
                tok.content += '\0'; // NULL terminated
                is_string = !is_string;
                state_old = state_new;
                continue; // Prevent last quote from being added 
                          // by skipping the iteration
            }

            is_string = !is_string;
        }

        if (c == '\n')
        {
            line++;
            col = 0;
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
        tok.span = { .line=line, .col=col, .len=len };
        len = 0;
        out.content.push_back(tok);
    }

    return out;
}

Lexer::Type Lexer::classify(std::string& content, CharType state)
{
    static const std::unordered_set<std::string> keywords = {
        "fn", "let", "comptime", "assert", "test", "if", "else", "extern", "return"
    };

    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/", "=>", ":", "==", "!=", ">=", "<=", ">", "<"
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
            // looking for ellipses specifically
            if (content == "...") return Type::Ellipsis;

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
            if (content == "=") return Type::Equal;
            if (operators.find(content) != operators.end())
                return Type::Operator;
            return Type::Invalid;

        case CharType::Quote:
            return Type::StringLiteral;
        
        case CharType::Caret:
            return Type::Caret;

        case CharType::Semi:
            return Type::Semi;

        case CharType::At:
            return Type::At;
        
        case CharType::Hash:
            return Type::Hash;
    }

    throw std::runtime_error(std::format(
        "This should not be reached!!!! Unknown state: {}", 
        (int)state
    ));
    return Type::Invalid;
}
