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
#include <iostream>
#include <vector>
#include <format>
#include <unordered_set>

#include <gearlang/error.hpp>
#include <gearlang/lex.hpp>

using namespace Error;

#define ANSI_RESET         "\x1b[0m"

#define ANSI_BOLD          "\x1b[1m"
#define ANSI_DIM           "\x1b[2m"
#define ANSI_ITALIC        "\x1b[3m"
#define ANSI_UNDERLINE     "\x1b[4m"
#define ANSI_BLINK         "\x1b[5m"
#define ANSI_REVERSE       "\x1b[7m"
#define ANSI_HIDDEN        "\x1b[8m"

#define ANSI_FG_BLACK      "\x1b[30m"
#define ANSI_FG_RED        "\x1b[31m"
#define ANSI_FG_GREEN      "\x1b[32m"
#define ANSI_FG_YELLOW     "\x1b[33m"
#define ANSI_FG_BLUE       "\x1b[34m"
#define ANSI_FG_MAGENTA    "\x1b[35m"
#define ANSI_FG_CYAN       "\x1b[36m"
#define ANSI_FG_WHITE      "\x1b[37m"

#define ANSI_FG_BRIGHT_BLACK   "\x1b[90m"
#define ANSI_FG_BRIGHT_RED     "\x1b[91m"
#define ANSI_FG_BRIGHT_GREEN   "\x1b[92m"
#define ANSI_FG_BRIGHT_YELLOW  "\x1b[93m"
#define ANSI_FG_BRIGHT_BLUE    "\x1b[94m"
#define ANSI_FG_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_FG_BRIGHT_CYAN    "\x1b[96m"
#define ANSI_FG_BRIGHT_WHITE   "\x1b[97m"

#define ANSI_BG_BLACK      "\x1b[40m"
#define ANSI_BG_RED        "\x1b[41m"
#define ANSI_BG_GREEN      "\x1b[42m"
#define ANSI_BG_YELLOW     "\x1b[43m"
#define ANSI_BG_BLUE       "\x1b[44m"
#define ANSI_BG_MAGENTA    "\x1b[45m"
#define ANSI_BG_CYAN       "\x1b[46m"
#define ANSI_BG_WHITE      "\x1b[47m"

std::unordered_set<ErrorCodes> codes;

std::ifstream input_file;
std::vector<std::string> error_split_file;
bool disable_color;

#define STYLE_HEADER(x) (disable_color ? "" : x)

#define ERROR_STYLE STYLE_HEADER(ANSI_BOLD ANSI_FG_BRIGHT_RED)
#define WARNING_STYLE STYLE_HEADER(ANSI_BOLD ANSI_FG_YELLOW)
#define SPAN_STYLE STYLE_HEADER(ANSI_BOLD ANSI_FG_MAGENTA)
#define MESSAGE_STYLE STYLE_HEADER(ANSI_BOLD)
#define RESET_STYLE STYLE_HEADER(ANSI_RESET)

void throw_error_base(
    Span const& span,
    const char* err,
    ErrorCodes code
) {
    std::string number = std::format("{}:{}", span.line, span.col);
    std::string highlight;

    for(size_t i = 0; i < span.col+number.size()+1; i++) highlight.push_back(' ');
    for(size_t i = 0; i < span.end-span.start; i++) highlight.push_back('^');

    std::cerr << ERROR_STYLE << "Error: " << RESET_STYLE << MESSAGE_STYLE << err << RESET_STYLE << '\n' <<
        SPAN_STYLE << span.line << ":" << span.col << ": " << RESET_STYLE << 
        error_split_file[span.line-1] << '\n' <<
        ERROR_STYLE << highlight << RESET_STYLE << std::endl;
}

void Error::throw_error_and_recover(
    Span const& span,
    const char* err,
    ErrorCodes code,
    Lexer::Stream& s
) {
    throw_error_base(span, err, code);
    codes.insert(code);

    while(s.has()) {
        auto curr = s.peek()->type;
        if(curr == Lexer::Type::Semi || curr == Lexer::Type::BraceClose) break;
        s.pop();
    }
}

void Error::flush() {
    if(codes.empty()) return;

    std::cerr << MESSAGE_STYLE << "Codes thrown: " << RESET_STYLE;

    for(auto it = codes.begin(); it != codes.end(); it++) {
        std::cerr << ERROR_STYLE << "E00" << (int)*it << RESET_STYLE;

        if(std::next(it) != codes.end()) {
            std::cerr << ", ";
        }
    }

    std::cerr << RESET_STYLE << std::endl;
}

void Error::throw_error (
    Span const& span,
    const char* err,
    ErrorCodes code
) {
    throw_error_base(span, err, code);
    codes.insert(code);
    flush();

    #ifdef ABORT_ON_FAIL
        abort();
    #else
        exit((int)code);
    #endif
}

void Error::throw_warning(
    Span const& span,
    const char* warning
) {
    std::string number = std::format("{}:{}", span.line, span.col);
    std::string highlight;

    for(size_t i = 0; i < span.col+number.size()+1; i++) highlight.push_back(' ');
    for(size_t i = 0; i < span.end-span.start; i++) highlight.push_back('^');
    
    std::cerr << WARNING_STYLE << "Warning: " << RESET_STYLE << MESSAGE_STYLE << warning << RESET_STYLE << '\n' <<
        SPAN_STYLE << span.line << ":" << span.col << ": " << RESET_STYLE << 
        error_split_file[span.line-1] << '\n' <<
        WARNING_STYLE << highlight << RESET_STYLE << std::endl;
}

void Error::setup_error_manager (const char* filename, bool _disable_color) {
    disable_color = _disable_color;

    input_file.open(filename);

    std::string line;
    while(std::getline(input_file, line)) {
        error_split_file.push_back(line);
    }
}
