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

unique_ptr<Expr> Expr::parse(Lexer::Stream& s) {
    return parseExpr(s);
}

ExprOp::Type match_op(string content) {
    using enum ExprOp::Type;
    
    if(content == "==") return Eq;
    if(content == "!=") return Ne;
    if(content == ">=") return Ge;
    if(content == "<=") return Le;
    throw std::runtime_error("Unknown type");
}


pExpr Expr::parseExpr(Lexer::Stream& s) {
    pExpr left = parseTerm(s);
    
    //no operator 
    if (s.peek()->type != Lexer::Type::Operator)
        return left;

    
    ExprOp::Type type;
    switch(s.peek()->content[0])
    {
        case '+': type = ExprOp::Type::Add; break;
        case '-': type = ExprOp::Type::Sub; break;
        case '*': type = ExprOp::Type::Mul; break;
        case '/': type = ExprOp::Type::Div; break;
        case '<': type = ExprOp::Type::Lt; break;
        case '>': type = ExprOp::Type::Gt; break;
        default: type = match_op(s.peek()->content);
    }

    s.pop();

    pExpr right = parseTerm(s);

    Span span = left->span_meta;
    span.end = right->span_meta.end;

    return std::make_unique<ExprOp>(type, std::move(left), std::move(right), span);
}

pExpr Expr::parseTerm(Lexer::Stream& s) {
    Span span = s.peek()->span;

    if (s.peek()->content == "(") {
        s.expect("(", span);
        auto expr = parseExpr(s);
        s.expect(")", span);
        return expr;
    }

    Lexer::Token lit = *(s.peek());

    switch (lit.type) {
        case Lexer::Type::FloatLiteral:   return ExprLitFloat::parse(s);    break;
        case Lexer::Type::IntegerLiteral: return ExprLitInt::parse(s);      break;
        case Lexer::Type::StringLiteral:  return ExprLitString::parse(s);   break;

        case Lexer::Type::At:   return ExprDeref::parse(s); break;
        case Lexer::Type::Hash: return ExprAddress::parse(s); break;
        default: break;
    }

    s.pop();

    if(lit.type == Lexer::Type::Identifier) {
        if(s.peek()->type == Lexer::Type::ParenOpen) {
            return ExprCall::parse(lit, s);
        }

        unique_ptr<ExprVar> var = ExprVar::parse(lit, s);

        if(s.peek()->type == Lexer::Type::Equal) {
            return ExprAssign::parse(std::move(var), s);
        }

        return var;
    }

    std::string error_msg = std::format("Unexpect token: {} (type={})", lit.content, (int)lit.type);
    
    Error::throw_error(
        span,
        "Unexpected token.",
        Error::ErrorCodes::UNEXPECTED_TOKEN
    );
}
