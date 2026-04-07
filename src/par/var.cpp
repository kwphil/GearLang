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
#include <string>
#include <format>

#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/vars.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::string;
using std::unique_ptr;

unique_ptr<ExprVar> ExprVar::parse(const Lexer::Token& token, Lexer::Stream& s) { 
    if(token.content.contains('.')) return ExprStructParam::parse(token, s);

    return std::make_unique<ExprVar>(token.content, token.span);
}

string ExprVar::to_string() { return std::format("{{ \"type\":\"ExprVar\", \"name\":\"{}\" }}", name); }

unique_ptr<ExprStructParam> ExprStructParam::parse(const Lexer::Token& token, Lexer::Stream& s) {
    Span span = token.span;
    auto split = split_string(token.content, '.');
    string struct_name = split[0];
    string param_name = split[1];
    
    return std::make_unique<ExprStructParam>(struct_name, param_name, span);
}

string ExprStructParam::to_string() { 
    return std::format(
        "{{ \"type\":\"ExprStructParam\", \"struct_name\":\"{}\", \"param_name\":\"{}\"}}",
        struct_name, name
    );
}

unique_ptr<Let> Let::parse(Lexer::Stream& s) {
    Span start_span = s.peek()->span;
    
    s.expect("let", start_span);
    string target = s.peek()->content;
    s.expect(Lexer::Type::Identifier);
    unique_ptr<Sem::Type> ty;

    if(s.peek()->content[0] == ':') {
        s.pop();
        ty = std::make_unique<Sem::Type>(s);
    }

    unique_ptr<Expr> expr;
    if(!ty || s.peek()->type == Lexer::Type::Equal) {
        s.expect("=", start_span);
        expr = Expr::parse(s);
    }

    Span new_span = start_span;
    new_span.end = s.peek()->span.end;

    return std::make_unique<Let>(
        target, std::move(expr), std::move(ty), new_span,
        check_keyword("export")
    );
}
