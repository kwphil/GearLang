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

#include <cctype>
#include <string>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_set>

#include <gearlang/lex.hpp>

using std::unordered_set;
using std::string;

unordered_set<string> keywords;
unordered_set<string> operators;

static char get_escape(char c) {
    switch(c) {
        case 'n': return '\n';
        case 't': return '\t';
        default:  return c;
    }
}

bool is_single_char_token(Lexer::CharType t);

unordered_set<string> split_string(const string& str, char delimiter) {
    unordered_set<string> tokens;
    std::stringstream ss(str);
    string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.insert(token);
    }
    return tokens;
}

Lexer::Stream Lexer::tokenize(const std::string& source_path) {
    std::ifstream file(source_path);
    Stream out;
    std::ifstream token_list("data/tokens.txt");
    assert(token_list.is_open());

    string buf;
    std::getline(token_list, buf);
    keywords = split_string(buf, ' ');
    std::getline(token_list, buf);
    operators = split_string(buf, ' ');

    char c;

    size_t index = 0;
    size_t line  = 1;
    size_t col   = 1;

    CharType state_old = CharType::Invalid;
    Token tok{};

    size_t token_start_index = 0;
    size_t token_start_line  = 1;
    size_t token_start_col   = 1;

    bool is_string  = false;
    bool is_comment = false;

    auto flush = [&](size_t end_index) {
        if(tok.content.empty())
            return;

        if(state_old == CharType::Invalid ||
           state_old == CharType::Format
        ) {
            tok.content.clear();
            return;
        }

        if(is_comment && !is_string) {
            tok.content.clear();
            return;
        }

        tok.span = {
            .line  = token_start_line,
            .col   = token_start_col,
            .start = token_start_index,
            .end   = end_index
        };

        tok.type = classify(tok.content, state_old, tok.span);

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
           state_new == CharType::Num
        ) {
            boundary = false;
        }

        if(boundary)
            flush(index);

        if(tok.content.empty() &&
           state_new != CharType::Format
        ) {
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
            is_string = !is_string;

            state_old = state_new;
            index++;
            col++;
            continue;
        }
        
        if(!is_string && tok.content == "/" && c == '/')
            is_comment = true;

        if(!is_comment || is_string)
            tok.content.push_back(c);

        state_old = state_new;

        index++;

        if(c == '\n') {
            line++;
            col = 1;
            is_comment = false;
        } else {
            col++;
        }
    }

    flush(index);

    return out;
}
