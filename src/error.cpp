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

#include <gearlang/error.hpp>

using namespace Error;

std::ifstream input_file;
std::vector<std::string> error_split_file;

void Error::throw_error (
    Span const& span,
    const char* err,
    ErrorCodes code
) {
    std::string number = std::format("{}:{}", span.line, span.col);
    std::string highlight;

    for(size_t i = 0; i < span.col+number.size()+1; i++) highlight.push_back(' ');
    for(size_t i = 0; i < span.end-span.start; i++) highlight.push_back('^');

    std::cerr << "Error: " << err << '\n' <<
        span.line << ":" << span.col << ": " << 
        error_split_file[span.line-1] << '\n' <<
        highlight << std::endl;

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

    std::cerr << "Warning: " << warning << '\n' <<
        span.line << ":" << span.col << ": " << 
        error_split_file[span.line-1] << '\n' <<
        highlight << std::endl;
}

void Error::setup_error_manager (const char* filename) {
    input_file.open(filename);

    std::string line;
    while(std::getline(input_file, line)) {
        error_split_file.push_back(line);
    }
}
