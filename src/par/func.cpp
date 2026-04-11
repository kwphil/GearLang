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

unique_ptr<Argument> Argument::parse(Lexer::Stream& s) {
    string arg = s.pop()->content;
    Sem::Type ty(s);

    return std::make_unique<Argument>(arg, ty, s.peek()->span);
}

string Argument::to_string() {
    return std::format(
        "{{ \"type\":\"Argument\", \"name\":\"{}\", \"ty\":\"{}\" }}",
        name,
        ty->dump()
    );
}

std::tuple<Sem::Type, string, deque<unique_ptr<Argument>>>
parse_function_header(
    Lexer::Stream& s,
    Span& span,
    bool requires_names,
    bool& is_variadic
);

deque<unique_ptr<Argument>>
parse_function_args(
    Lexer::Stream& s,
    Span& span,
    bool requires_names,
    bool& is_variadic,
    Sem::Type& ty
);

unique_ptr<Function> Function::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    bool is_variadic;

    s.expect("fn", span);

    auto [ty, name, args] =
        parse_function_header(s, span, false, is_variadic);

    auto block = NodeBase::parse(s);

    return std::make_unique<Function>(
        name,
        ty,
        std::move(args),
        std::move(block),
        is_variadic,
        span,
        check_keyword("export")
    );
}

string Function::to_string() {
    string args_s;

    for(size_t i = 0; i < args.size(); i++) {
        args_s += args[i]->to_string();
        if(i + 1 != args.size())
            args_s += ", ";
    }

    return std::format(
        "{{ \"type\":\"Function\", \"name\":\"{}\", \"ty\":\"{}\", "
        "\"args\":[{}], \"block\":{}, \"is_variadic\":{} }}",
        name,
        ty.dump(),
        args_s,
        block->to_string(),
        is_variadic
    );
}

unique_ptr<ExternFn> ExternFn::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;

    bool is_variadic;
    bool no_mangle = false;

    s.expect("extern", span);

    if(s.peek()->type == Lexer::Type::ParenOpen) {
        s.pop();

        auto tok = s.pop();

        if(tok->type != Lexer::Type::StringLiteral) {
            Error::throw_error_and_recover(
                tok->span,
                std::format(
                    "Expected StringLiteral, received {}",
                    Lexer::print_type(tok->type)
                ).c_str(),
                Error::ErrorCodes::UNEXPECTED_TOKEN, s
            );
        }

        if(strcmp(tok->content.c_str(), "C") == 0) {
            no_mangle = true;
            s.expect(")", s.peek()->span);
        } else {
            throw std::runtime_error(tok->content);
        }
    }

    s.expect("fn", span);

    auto [ty, name, args] =
        parse_function_header(s, span, false, is_variadic);

    return std::make_unique<ExternFn>(
        name,
        ty,
        std::move(args),
        is_variadic,
        no_mangle,
        span
    );
}

string ExternFn::to_string() {
    string args_s;

    for(size_t i = 0; i < args.size(); i++) {
        args_s += args[i]->to_string();
        if(i + 1 != args.size())
            args_s += ", ";
    }

    return std::format(
        "{{ \"type\":\"ExternFn\", \"name\":\"{}\", \"ty\":\"{}\", "
        "\"args\":[{}], \"is_variadic\":{}, \"no_mangle\":{} }}",
        callee,
        ty.dump(),
        args_s,
        is_variadic,
        no_mangle
    );
}
