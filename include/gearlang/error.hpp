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

#include "etc.hpp"

namespace Error {
    enum class ErrorCodes {
        reserved0,
        EXPECT_VALUE,
        UNEXPECTED_TOKEN,
        UNEXPECTED_EOF,
        UNKNOWN_TYPE,
        VARIABLE_ALREADY_DEFINED,
        VARIABLE_NOT_DEFINED,
        FUNCTION_NOT_DEFINED,
        FUNCTION_INVALID_ARGS,
        QUALIFIER_NOT_ALLOWED,
        INVALID_AST,
        BAD_TYPE,
    };

    /// @brief throws an error based on a span. 
    /// @param span the span to throw at
    /// @param err a specific message to throw
    /// @param code the error code to throw
    [[noreturn]] void throw_error (
        Span const& span,
        const char* err,
        ErrorCodes code
    );

    /// @brief Throws a warning based on a span.
    /// @param span the span to throw at
    /// @param err a specific message to throw
    void throw_warning (
        Span const& span,
        const char* err
    );

    /// @brief Sets up the error management system
    /// @param filename the name of the file to open
    void setup_error_manager (const char* filename);
}
