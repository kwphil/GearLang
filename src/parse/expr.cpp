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

unique_ptr<ExprLitInt> ExprLitInt::parse(Lexer::Stream& s) { 
    auto tok = *s.pop();

    return std::make_unique<ExprLitInt>((uint64_t)std::stoi(tok.content), tok.span); 
}

string ExprLitInt::to_string() { return std::format("{{ \"type\":\"ExprLitInt\", \"value\":{} }}", value); }

unique_ptr<ExprLitFloat> ExprLitFloat::parse(Lexer::Stream& s) { 
    auto tok = *s.pop();
    double val = std::stod(tok.content);

    return std::make_unique<ExprLitFloat>(val, tok.span); 
}

string ExprLitFloat::to_string() { return std::format("{{ \"type\":\"ExprLitFloat\", \"value\":{} }}", value); }

unique_ptr<ExprLitString> ExprLitString::parse(Lexer::Stream& s) {
    unique_ptr<Lexer::Token> t = s.pop();

    return std::make_unique<ExprLitString>(t->content, t->span);
}

string ExprLitString::to_string() { 
    std::string out;

    for(auto c : string) {
        if(c == '\n') {
            out.push_back('\\');
            out.push_back('n');
            continue;
        }
        if(c == '\t') {
            out.push_back('\\');
            out.push_back('t');
            continue;
        }

        out.push_back(c);
    }

    return std::format("{{ \"type\":\"ExprLitString\", \"string\":\"{}\" }}", out); 
}

unique_ptr<ExprVar> ExprVar::parse(const Lexer::Token& token, Lexer::Stream& s) { 
    if(token.content.back() == '.') return ExprStructParam::parse(token, s);

    return std::make_unique<ExprVar>(token.content, token.span);
}

string ExprVar::to_string() { return std::format("{{ \"type\":\"ExprVar\", \"name\":\"{}\" }}", name); }

unique_ptr<ExprStructParam> ExprStructParam::parse(const Lexer::Token& token, Lexer::Stream& s) {
    Span span = token.span;
    string struct_name = token.content;
    struct_name.pop_back();
    string param_name = s.peek()->content;
    span.end = s.pop()->span.end;
    
    return std::make_unique<ExprStructParam>(struct_name, param_name, span);
}

string ExprStructParam::to_string() { 
    return std::format(
        "{{ \"type\"=\"ExprStructParam\", \"struct_name\":\"{}\", \"param_name\":\"{}\"}}",
        struct_name, name
    );
}

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

string ExprOp::to_string() {
    return std::format("{{ \"type\":\"ExprOp\", \"oper\":{}, \"left\":{}, \"right\":{} }}", 
        (int)type, left->to_string(), right->to_string()
    );
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
