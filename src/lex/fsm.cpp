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

    size_t index = 0;  
    size_t line  = 1;
    size_t col   = 1;

    CharType state_old = CharType::Invalid;
    Token tok{};

    // Token start metadata
    size_t token_start_index = 0;
    size_t token_start_line  = 1;
    size_t token_start_col   = 1;

    bool is_string  = false;
    bool is_comment = false;

    auto flush = [&](size_t end_index)
    {
        if (tok.content.empty())
            return;

        if (state_old == CharType::Invalid ||
            state_old == CharType::Format)
        {
            tok.content.clear();
            return;
        }

        if (is_comment && !is_string)
        {
            tok.content.clear();
            return;
        }

        tok.type = classify(tok.content, state_old);
        tok.span = {
            .line  = token_start_line,
            .col   = token_start_col,
            .start = token_start_index,
            .end   = end_index
        };

        out.content.push_back(tok);
        tok = Token{};
    };

    while (file.get(c))
    {
        CharType state_new = getCharType(c);

        // determine if this character causes a token boundary
        bool boundary =
            !is_string &&
            (
                state_new != state_old ||
                is_single_char_token(state_new) ||
                is_single_char_token(state_old)
            );

        // allow identifiers like "num1"
        if (!is_string &&
            state_old == CharType::Alpha &&
            state_new == CharType::Num)
        {
            boundary = false;
        }

        if (boundary)
        {
            flush(index);
        }

        if (tok.content.empty() &&
            state_new != CharType::Format)
        {
            token_start_index = index;
            token_start_line  = line;
            token_start_col   = col;
        }

        if (is_string && c == '\\')
        {
            char next;
            if (file.get(next))
            {
                tok.content.push_back(get_escape(next));

                index++;
                col++;

                state_old = state_new;
                continue;
            }
        }

        if (state_new == CharType::Quote)
        {
            is_string = !is_string;

            state_old = state_new;
            index++;
            col++;
            continue;
        }

        if (!is_string && tok.content == "/" && c == '/')
        {
            is_comment = true;
        }

        if (!is_comment || is_string)
        {
            tok.content.push_back(c);
        }

        state_old = state_new;

        index++;

        if (c == '\n')
        {
            line++;
            col = 1;
            is_comment = false;
        }
        else
        {
            col++;
        }
    }

    flush(index);

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
