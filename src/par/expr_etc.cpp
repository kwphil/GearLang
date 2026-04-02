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
#include <vector>
#include <format>

#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/vars.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::string;
using std::unique_ptr;

unique_ptr<ExprAssign>
ExprAssign::parse(unique_ptr<ExprVar> var, Lexer::Stream& s) {
    Span span = var->span_meta;

    s.expect("=", span);
    pExpr expr = Expr::parse(s);

    span.end = expr->span_meta.end;
    return std::make_unique<ExprAssign>(std::move(var), std::move(expr), span);
}

string ExprAssign::to_string() { 
    return std::format("{{ \"type\":\"ExprAssign\", \"var\":{}, \"expr\":{} }}", var->to_string(), expr->to_string()); 
}

string oper_string(ExprOp::Type ty) {
    using enum ExprOp::Type;
    
    switch(ty) {
        case Add: return "Add";
        case Sub: return "Sub";
        case Mul: return "Mul";
        case Div: return "Div";
        case Gt: return "Gt";
        case Lt: return "Lt";
        case Ge: return "Ge";
        case Le: return "Le";
        case Eq: return "Eq";
        case Ne: return "Ne";
        default: return "Invalid";
    }
}

string ExprOp::to_string() {
    return std::format("{{ \"type\":\"ExprOp\", \"oper\":\"{}\", \"left\":{}, \"right\":{} }}", 
        oper_string(type), left->to_string(), right->to_string()
    );
}

unique_ptr<ExprCall> ExprCall::parse(
    const Lexer::Token& tok,
    Lexer::Stream& s
) {
    Span span = tok.span;
    std::string nm = tok.content;
    std::vector<pExpr> args;

    s.expect("(", span);
    
    while(s.peek()->type != Lexer::Type::ParenClose) {
        args.push_back(Expr::parse(s));

        // Looking for the ) the loop will end anyway
        if(s.peek()->type != Lexer::Type::ParenClose) {
            s.expect(",", span);
        }
    }

    s.expect(")", span);

    return std::make_unique<ExprCall>(nm, args, span);
}

string ExprCall::to_string() {
    string args_str;

    for(auto& n : args) {
        args_str += n->to_string();
        args_str.push_back(',');
    }

    args_str.pop_back();

    return std::format("{{ \"type\":\"ExprCall\", \"callee\":\"{}\", \"args\":[{}] }}", callee, args_str);
}

unique_ptr<ExprAddress> ExprAddress::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;

    s.expect("#", span);

    auto tok = s.pop();
    span.end = tok->span.end;
    return std::make_unique<ExprAddress>(std::move(ExprVar::parse(*tok, s)), span);
}

string ExprAddress::to_string() { return std::format("{{ \"type\":\"ExprAddress\", \"var\":{} }}", var->to_string()); }

unique_ptr<ExprDeref> ExprDeref::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;

    s.expect("@", span);
    unique_ptr<Lexer::Token> var = s.pop();
    span.end = var->span.end;
    return std::make_unique<ExprDeref>(std::move(ExprVar::parse(*var, s)), span);
}

string ExprDeref::to_string() { return std::format("{{ \"type\":\"ExprDeref\", \"var\":{} }}", var->to_string()); }
