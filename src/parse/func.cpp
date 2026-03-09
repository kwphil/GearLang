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
using std::vector;

unique_ptr<Argument> Argument::parse(Lexer::Stream& s) {
    string arg = s.pop()->content;
    Sem::Type* ty = new Sem::Type(s); 

    return std::make_unique<Argument>(arg, ty, s.peek()->span);
}

string Argument::to_string() { 
    return std::format("{{ \"type\":\"Argument\", \"name\":\"{}\", \"ty\":\"{}\" }}",
        name, ty->dump()
    ); 
}

// Helper function for parsing function headers
// Returns the type 
// Returns the name of the function
// Returns the arguments
std::tuple<Sem::Type, string, deque<unique_ptr<Argument>>> 
parse_function_header(
    Lexer::Stream& s, 
    Span& span,
    bool requires_names,
    bool& is_variadic
);

// Helper function
// Parses the arguments for a function
deque<unique_ptr<Argument>> parse_function_args(
    Lexer::Stream& s, 
    Span& span,
    bool requires_names, // TODO: Implement this
    bool& is_variadic,
    Sem::Type& ty
);

std::unique_ptr<Function>
Function::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    bool is_variadic;

    s.expect("fn", span);

    Sem::Type ty;
    string name;
    deque<unique_ptr<Argument>> args;

    auto header = parse_function_header(s, span, false, is_variadic);

    ty = std::get<0>(header);
    name = std::get<1>(header);
    args = std::move(std::get<2>(header));

    auto block = NodeBase::parse(s);
    return std::make_unique<Function>(name, ty, std::move(args), std::move(block), is_variadic, span);
}

string Function::to_string() {
    string args_s;
    
    for(auto& arg : args) {
        args_s += arg->to_string();
        args_s += ", ";
    }

    args_s = args_s.substr(0, args_s.size()-2);

    return std::format(
        "{{ \"type\":\"Function\", \"name\":\"{}\", \"ty\":\"{}\", "
        "\"args\":[{}], \"block\":{}, \"is_variadic\":{} }}",
        name, ty.dump(), args_s, block->to_string(), is_variadic
    );
}

std::unique_ptr<ExternFn> ExternFn::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    bool is_variadic;
    bool no_mangle = false;
    
    s.expect("extern", span);
    
    if(s.peek()->type == Lexer::Type::ParenOpen) {
        s.pop();

        unique_ptr<Lexer::Token> tok = s.pop();
        const char* str = tok->content.c_str();

        if(tok->type != Lexer::Type::StringLiteral) {
            Error::throw_error(
                tok->span,
                std::format("Expected a StringLiteral, received {}", 
                    Lexer::print_type(tok->type)
                ).c_str(),
                Error::ErrorCodes::UNEXPECTED_TOKEN
            );
        }

        // Using strcmp because my strings have NULLs
        // that screw everything up with string::operator==
        if(strcmp(str, "C") == 0) {
            no_mangle = true;
            s.expect(")", s.peek()->span);
            goto after_extern;
        } 

        throw std::runtime_error(s.peek()->content);
    }
after_extern:

    s.expect("fn", span);

    Sem::Type ty;
    string name;
    deque<unique_ptr<Argument>> args;

    auto header = parse_function_header(s, span, false, is_variadic);

    ty = std::get<0>(header);
    name = std::get<1>(header);
    args = std::move(std::get<2>(header));

    return std::make_unique<ExternFn>(name, ty, args, is_variadic, no_mangle, span);
}

string ExternFn::to_string() {
    string args_s;
    
    for(auto& arg : args) {
        args_s += arg->to_string();
        args_s += ", ";
    }

    args_s = args_s.substr(0, args_s.size()-2);

    return std::format(
        "{{ \"type\":\"ExternFn\", \"name\":\"{}\", \"ty\":\"{}\", \"args\":[{}], \"is_variadic\":{}, \"no_mangle\":{} }}",
        callee, ty.dump(), args_s, is_variadic, no_mangle
    );
}

// PRIVATE FUNCTIONS


std::tuple<Sem::Type, string, deque<unique_ptr<Argument>>> 
parse_function_header(
    Lexer::Stream& s, 
    Span& span,
    bool requires_names,
    bool& is_variadic
) {
    Sem::Type ty;
    std::string name = s.pop()->content;

    deque<unique_ptr<Argument>> args =
        parse_function_args(s, span, requires_names, is_variadic, ty);

    return { ty, name, std::move(args) };
}

deque<unique_ptr<Argument>> parse_function_args(
    Lexer::Stream& s, 
    Span& span,
    bool requires_names, // TODO: Implement this
    bool& is_variadic,
    Sem::Type& ty
) {
    is_variadic = false;
    deque<unique_ptr<Argument>> args;

    // No arguments
    if (s.peek()->content != "(") {
        goto after_args;
    }

    // Consume '('
    s.pop();

    while (true) {
        if (!s.has()) {
            Error::throw_error(
                span,
                "Unexpected EOF parsing args for function",
                Error::ErrorCodes::UNEXPECTED_EOF
            );
        }

        if (s.peek()->type == Lexer::Type::Ellipsis) {
            s.pop(); // Get rid of the ellipsis
            is_variadic = true;
            break; // Will be the last parameter
        }
        
        args.push_back(Argument::parse(s));
        
        if (s.peek()->content == ")") break;
        s.expect(",", s.peek()->span);
        // One more to allow an optional final comma
        if(s.peek()->content == ")") break;
    }
    
    s.pop(); // )

after_args:
    ty = Sem::Type("void");
    if(s.peek()->content == "returns") {
        s.pop();
        ty = Sem::Type(s);
    }

    return args;
}
