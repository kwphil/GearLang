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

#include <string>
#include <vector>
#include <memory>
#include <format>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::unique_ptr;
using std::string;

unique_ptr<Let> Let::parse(Lexer::Stream& s) {
    Span start_span = s.peek()->span;
    
    s.expect("let", start_span);
    string target = s.pop()->content;
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

    return std::make_unique<Let>(target, std::move(expr), std::move(ty), new_span);
}

string Let::to_string() {
    return std::format(
        "{{ let target={} expr={} }}",
        target, expr.value()->to_string()
    );
}

unique_ptr<ExprBlock> ExprBlock::parse(Lexer::Stream& s) {
    Span start_span = s.peek()->span;
    std::vector<unique_ptr<NodeBase>> nodes;

    s.expect("{", start_span);
    unique_ptr<Lexer::Token> t = s.peek();
    int brace_count = 1;
    while(s.has()) {
        // For nested braces
        if(t->type == Lexer::Type::BraceOpen) brace_count++;
        if(t->type == Lexer::Type::BraceClose) brace_count--;

        if(brace_count == 0) {
            break;
        }

        nodes.push_back(NodeBase::parse(s));

        t = s.peek();
    }

    Span new_span = start_span;
    start_span.end = s.pop()->span.end; // Remove the }

    return std::make_unique<ExprBlock>(std::move(nodes), new_span);
}

string ExprBlock::to_string() {
    string out = "{ ExprBlock ";

    for(auto& n : nodes) {
        out += " { ";
        out += n->to_string();
        out += " } ";
    }

    out += " }";

    return out;
}

unique_ptr<NodeBase> NodeBase::parse(Lexer::Stream& s) {
    unique_ptr<NodeBase> out;
    unique_ptr<Lexer::Token> curr = s.peek();
    Span span = curr->span;

    if(!s.has()) {
        Error::throw_error(
            span,
            "Unexpected EOF",
            Error::ErrorCodes::UNEXPECTED_EOF
        );
    }
    
    // These do not require semicolons, so early return
    if      (curr->content == "fn")     return Function::parse(s);
    else if (curr->content == "{")      return ExprBlock::parse(s);
    else if (curr->content == "if") {
        auto if_expr = If::parse(s);
        
        if(s.peek()->content == "else") {
            return Else::parse(std::move(if_expr), s);
        } 

        return if_expr;
    }
    // These do
    else if (curr->content == "let")    out = Let::parse(s);
    else if (curr->content == "return") out = Return::parse(s);
    else if (curr->content == "extern") out = ExternFn::parse(s);
    else                                out = Expr::parse(s);
    s.expect(";", s.peek()->span);

    return out;
}

unique_ptr<Return> Return::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    
    s.expect("return", span);
    unique_ptr<Expr> expr = Expr::parse(s);

    span.end = expr->span_meta.end;
    return std::make_unique<Return>(std::move(expr), span);
}

string Return::to_string() { return std::format("{{ Return expr={} }}", expr->to_string()); }

unique_ptr<If> If::parse(Lexer::Stream& s) {
    Span span = s.peek()->span; // Get it here on the line of the if statement itself
    s.expect("if", span);
    pExpr cond = Expr::parse(s);
    unique_ptr<NodeBase> expr = NodeBase::parse(s);

    span.end = expr->span_meta.end;
    return std::make_unique<If>(std::move(expr), std::move(cond), span);
}

string If::to_string() {
    return std::format("{{ if cond={} expr={} }}", cond->to_string(), expr->to_string());
}

unique_ptr<Else> Else::parse(
    unique_ptr<If> if_expr,
    Lexer::Stream& s
) {
    Span span = s.peek()->span;
    s.expect("else", span);
    unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<Else>(std::move(expr), std::move(*if_expr));
}

string Else::to_string() {
    return std::format("{{ else cond={} exprtrue={} exprfalse={} }}",
        cond->to_string(), expr->to_string(), else_expr->to_string()
    );
}

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has()) {
        that.content.push_back(Nodes::NodeBase::parse(s));
    }
    
    return that; 
}

std::string Ast::Program::to_string() {
    std::string out;
    
    for(auto& n : content) {
        out += n->to_string();
        out += '\n';
    }   

    return out;
}
