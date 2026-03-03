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
#include <fstream>
#include <vector>

namespace Error {
    enum class ErrorCodes {
        EXPECT_VALUE,
        UNEXPECTED_TOKEN,
        UNEXPECTED_EOF,
        UNKNOWN_TYPE,
        VARIABLE_ALREADY_DEFINED,
        VARIABLE_NOT_DEFINED,
        FUNCTION_NOT_DEFINED,
        INVALID_AST,
        BAD_TYPE,
    };

    /// @brief throws an error at a line. noreturn just to suppress warning about no func return.
    /// @param line_number the line to throw an error
    /// @param search_for will highlight a specific section of text. If it fails, it will search `leniency` lines down for it. If it still doesn't work, it will just not highlight anything. Pass "" to disable the feature
    /// @param err a specific message to throw
    /// @param leniency the number of lines to search through in case search_for fails
    /// @param code the error code to throw. .
    [[noreturn]] void throw_error (
        int line_number, 
        const char* search_for, 
        const char* err,
        ErrorCodes code,
        unsigned int leniency = 5
    );

    /// @brief Sets up the error management system
    /// @param filename the name of the file to open
    void setup_error_manager (const char* filename);
}
