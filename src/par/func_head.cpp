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
#include <memory>
#include <deque>
#include <tuple>
#include <format>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::string;
using std::unique_ptr;
using std::deque;

deque<unique_ptr<Argument>>
parse_function_args(
    Lexer::Stream& s,
    Span& span,
    bool requires_names,
    bool& is_variadic
) {
    is_variadic = false;
    deque<unique_ptr<Argument>> args;

    if(s.peek()->content == "(") {
        bool loop = true; // Quit if no args are present within the parens

        s.pop();

        if(s.peek()->type == Lexer::Type::ParenClose) {
            loop = false;
        }

        while(loop) {
            if(!s.has()) {
                Error::throw_error_and_recover(
                    span,
                    "Unexpected EOF parsing function args",
                    Error::ErrorCodes::UNEXPECTED_EOF, s
                );
            }

            if(s.peek()->type == Lexer::Type::Ellipsis) {
                s.pop();
                is_variadic = true;
                break;
            }

            if(requires_names && s.next()->content != ":") {
                Error::throw_error(span, "Function definitions require names", Error::ErrorCodes::FUNCTION_INVALID_ARGS);
            }

            args.push_back(Argument::parse(s, requires_names));

            if(s.peek()->content == ")")
                break;

            s.expect(",", s.peek()->span);

            if(s.peek()->content == ")")
                break;
        }

        s.pop(); // ')'
    }

    return args;
}

std::tuple<Sem::Type, string, deque<unique_ptr<Argument>>, ManglingScheme>
parse_function_header(
    Lexer::Stream& s,
    Span& span,
    bool requires_names,
    bool& is_variadic
) {
    string name = s.pop()->content;
    Sem::Type ty = Sem::Type("void");

    auto args =
        parse_function_args(s, span, requires_names, is_variadic);

    if(s.peek()->content == "returns") {
        s.pop();
        span.end = s.peek()->span.end;
        ty = Sem::Type(s);
    }

    auto scheme = ManglingScheme::Gearlang;

    span.end = s.peek()->span.end;

    if(s.peek()->content == "mangle") {
        s.pop();

        s.expect(Lexer::Type::ParenOpen);

        auto mode = s.pop()->content;

        if(mode == "C") {
            scheme = ManglingScheme::None;
        } else if (mode == "C++") {
            scheme = ManglingScheme::Itanium;
        } else if (mode == "Gear") {
            scheme = ManglingScheme::Gearlang;
        }

        span.end = s.peek()->span.end;
        s.expect(Lexer::Type::ParenClose);
    }

    if(name.contains('.') && scheme != ManglingScheme::Itanium) {
        Error::throw_error(
            span,
            "Non-C++ function declarations cannot contain a `.`",
            Error::ErrorCodes::INVALID_AST
        );
    }

    return { ty, name, std::move(args), scheme };
}
