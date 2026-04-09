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

#include "gearlang/error.hpp"
#include <string>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <filesystem>

#include <gearlang/lex.hpp>

using std::unordered_set;
using std::string;

namespace fs = std::filesystem;

static char get_escape(char c) {
    switch(c) {
        case 'n': return '\n';
        case 't': return '\t';
        default:  return c;
    }
}

bool is_single_char_token(Lexer::CharType t);

Lexer::Stream Lexer::tokenize(const std::string& source_path) {
    std::ifstream file(source_path);

    std::string out;
    std::string buf; 
    while(std::getline(file, buf)) out += buf + '\n';
    return tokenize_by_string(out, fs::canonical(source_path));
}

Lexer::Stream Lexer::tokenize_by_string(std::string& str, std::string file_name) {
    std::stringstream file;
    file << str;
    Stream out;

    std::ifstream token_list("data/tokens.txt");
    assert(token_list.is_open());

    string buf;
    Table table;
    std::getline(token_list, buf);
    auto spl = split_string(buf, ' ');
    table.keywords = unordered_set<string>(spl.begin(), spl.end());
    std::getline(token_list, buf);
    spl = split_string(buf, ' ');
    table.operators = unordered_set<string>(spl.begin(), spl.end());

    char c;

    size_t index = 0;
    size_t line  = 1;
    size_t col   = 1;

    CharType state_old = CharType::Invalid;
    Token tok{};

    size_t token_start_index = 0;
    size_t token_start_line  = 1;
    size_t token_start_col   = 1;

    bool token_is_string = false;
    bool is_string  = false;
    bool is_comment = false;    
    bool is_block_comment = false;  

    auto flush = [&](size_t end_index) {
        if(tok.content.empty() && !token_is_string) // strings can be empty
            return;

        if(
            (
                state_old == CharType::Invalid ||
                state_old == CharType::Format
            ) && !is_string
        ) {
            tok.content.clear();
            return;
        }

        if(is_comment && !is_string) {
            tok.content.clear();
            return;
        }

        tok.span = {
            .file  = file_name,
            .line  = token_start_line,
            .col   = token_start_col,
            .start = token_start_index,
            .end   = end_index
        };

        if (token_is_string) {
            tok.type = Type::StringLiteral;
        } else {
            tok.type = classify(tok.content, state_old, tok.span, table);
        }

        out.content.push_back(tok);
        tok = Token{};
    };

    while(file.get(c)) {
        CharType state_new = getCharType(c);

        bool boundary =
            !is_string &&
            (
                state_new != state_old ||
                is_single_char_token(state_new) ||
                is_single_char_token(state_old)
            );

        if(!is_string &&
        state_old == CharType::Alpha &&
        state_new == CharType::Num) boundary = false;

        if(!is_string &&
        state_old == CharType::Num &&
        state_new == CharType::Alpha) boundary = false;

        if(boundary) flush(index);

        if(tok.content.empty() &&
        state_new != CharType::Format) {
            token_start_index = index;
            token_start_line  = line;
            token_start_col   = col;
        }

        if(is_string && c == '\\') {
            char next;
            if(file.get(next)) {
                tok.content.push_back(get_escape(next));
                index++;
                col++;
                state_old = state_new;
                continue;
            }
        }

        if(state_new == CharType::Quote) {
            if(is_comment || is_block_comment) {
                continue;
                index++;
                col++;
            }

            if(!is_string) {
                tok = Token{};
                token_is_string = true;
                token_start_index = index;
                token_start_line  = line;
                token_start_col   = col;
            } else {
                flush(index);
                token_is_string = false;
            }
            is_string = !is_string;
            state_old = state_new;
            index++; col++;
            continue;
        }

        if(!is_string) {
            if(!is_comment && !is_block_comment && tok.content == "/" && c == '/') {
                is_comment = true;
            } else if(!is_comment && !is_block_comment && tok.content == "/" && c == '*') {
                is_block_comment = true;
                tok.content.clear(); 
            } else if(is_block_comment && tok.content == "*" && c == '/') {
                is_block_comment = false;
                tok.content.clear(); 
                state_old = CharType::Invalid;
                index++; col++;
                continue;
            }
        }

        if((!is_comment && !is_block_comment) || is_string)
            tok.content.push_back(c);

        state_old = state_new;
        index++;

        if(c == '\n') {
            line++;
            col = 1;
            is_comment = false;
            if(is_string) {
                Span span { .file=file_name, .line=token_start_line, .col=token_start_col, .start=token_start_index, .end=index };
                Error::throw_error(span, "Unterminated string", Error::ErrorCodes::UNEXPECTED_TOKEN);
            }
        } else {
            col++;
        }
    }

    if(is_string) {
        Span span { .file=file_name, .line=token_start_line, .col=token_start_col, .start=token_start_index, .end=index };
        Error::throw_error(span, "Unterminated string", Error::ErrorCodes::UNEXPECTED_TOKEN);
    }

    flush(index);

    return out;
}