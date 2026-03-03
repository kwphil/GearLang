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

#include <gearlang/error.hpp>

std::ifstream input_file;
std::vector<std::string> error_split_file;

void throw_error_no_highlight(
    int line_number,
    const char* err,
    Error::ErrorCodes code
) {
    // Print the error
    std::cerr << "Error: " << err << "\n";
    // Print highlighted line
    std::cerr << error_split_file[line_number];

    // Flush cerr
    std::cerr << std::endl;

    exit((int)code);
}

void Error::throw_error (
    int line_number, // This one is a copy so I'll be modifying it a little
    const char* _search_for, 
    const char* err,
    ErrorCodes code,
    unsigned int leniency
) {
    std::string search_for = _search_for;
    // if nothing to highlight
    if(search_for == "") {
        throw_error_no_highlight(line_number, err, code);
    }

    std::string current_line;
    long unsigned int loc;
    
    // Looking for search_for
    for(unsigned int i = 0; i < leniency+1; i++) {
        current_line = error_split_file[line_number-1];
        loc = current_line.find(search_for);
        if(loc != std::string::npos) break;
        line_number++;
    }

    // If the program still can't find the contents, 
    // just call without highlights
    if(loc == std::string::npos) {
        std::cerr << "COMPILER WARNING: compiler could not find a match "
        "to string: " << search_for << "\n";
        throw_error_no_highlight(
            line_number,
            err,
            code
        );
    }

    // Print error
    std::cerr << "Error: " << err << "\n";

    // Print line
    std::cerr << current_line << "\n";
    
    // Offset highlight
    for(unsigned int i = 0; i < loc; i++) {
        std::cerr << " ";
    }

    // Print highlight
    for(unsigned int i = 0; i < search_for.length(); i++) {
        std::cerr << "^";
    }

    // flush cerr
    std::cerr << std::endl;

    exit((int)code);
}

void Error::setup_error_manager (const char* filename) {
    input_file.open(filename);

    std::string line;
    while(std::getline(input_file, line)) {
        error_split_file.push_back(line);
    }
}