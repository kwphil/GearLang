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

    return std::make_unique<Argument>(arg, ty, s.peek()->span.line);
}

string Argument::to_string() { 
    return std::format("{{ Argument, name={}, ty={} }}",
        name, ty->dump()
    ); 
}

// Helper function for parsing function headers
// Returns the type (returns Ast::Type::NonPrimitive, ... if non-primitive)
// Returns the name of the function
std::tuple<Sem::Type, string> 
parse_function_header(Lexer::Stream& s, int line_number);

// Helper function
// Parses the arguments for a function
deque<unique_ptr<Argument>> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names,
    bool& is_variadic
);

std::unique_ptr<Function>
Function::parse(Lexer::Stream& s) {
    int line_number = s.peek()->span.line;
    bool is_variadic;

    s.expect("fn", line_number);

    Sem::Type ty;
    string name;
    deque<unique_ptr<Argument>> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, true, is_variadic);

    auto block = NodeBase::parse(s);
    return std::make_unique<Function>(name, ty, std::move(args), std::move(block), is_variadic, line_number);
}

string Function::to_string() {
    string args_s;
    
    for(auto& arg : args) {
        args_s += arg->to_string();
    }

    return std::format(
        "{{ Function name={}, ty={}, args={}, block={}, is_variadic={} }}",
        name, ty.dump(), args_s, block->to_string(), is_variadic
    );
}

std::unique_ptr<ExternFn> ExternFn::parse(Lexer::Stream& s) {
    int line_number = s.peek()->span.line;
    bool is_variadic;
    bool no_mangle = false;
    
    s.expect("extern", line_number);
    
    if(s.peek()->type == Lexer::Type::StringLiteral) {
        // Using strcmp because my strings have NULLs
        // that screw everything up with string::operator==
        if(strcmp(s.peek()->content.c_str(), "C") == 0) {
            no_mangle = true;
            s.pop();
            goto after_extern;
        } 

        throw std::runtime_error(s.peek()->content);
    }
after_extern:

    s.expect("fn", line_number);

    Sem::Type ty;
    string name;
    deque<unique_ptr<Argument>> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, false, is_variadic);

    return std::make_unique<ExternFn>(name, ty, args, is_variadic, no_mangle, line_number);
}

string ExternFn::to_string() {
    string args_s;
    
    for(auto& arg : args) {
        args_s += arg->to_string();
    }

    return std::format(
        "{{ Function name={}, ty={}, args={}, is_variadic={}, no_mangle={} }}",
        callee, ty.dump(), args_s, is_variadic, no_mangle
    );
}

// PRIVATE FUNCTIONS


std::tuple<Sem::Type, string> 
parse_function_header(Lexer::Stream& s, int line_number) {
    // Skipping over the current token, is there a parameter list or a block? 
    // If there is not, then there is a type included
    Sem::Type ty;
    
    auto t = s.peek();
    if(s.next()->content != ":" && s.next()->type != Lexer::Type::BraceOpen) {
        ty = Sem::Type(s);
    }

    std::string name = s.pop()->content;

    return { ty, name };
}

deque<unique_ptr<Argument>> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names, // TODO: Implement this
    bool& is_variadic
) {
    is_variadic = false;

    // No arguments
    if (s.peek()->content != ":") {
        return { };
    }

    // Consume ':'
    s.pop();

    deque<unique_ptr<Argument>> args;

    while (true) {
        if (!s.has()) {
            Error::throw_error(
                line_number,
                "",
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
        
        if (s.peek()->content == "{" || s.peek()->content == ";") break;
        s.expect(",", line_number);
    }

    return args;
}
