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
#include <vector>
#include <string>
#include <format>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/branch.hpp>

using namespace Ast::Nodes;
using Sem::Type;
using std::unique_ptr;
using std::string;
using std::vector;

unique_ptr<Struct> Struct::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;

    string name = s.next()->content;
    Sem::Type ty(s);

    return std::make_unique<Struct>(name, ty, span);
}

std::string Struct::to_string() {
    return "{ \"type\":\"Struct\", \"name\":\"\" }";
}

string Let::to_string() {
    return std::format(
        "{{ \"type\":\"Let\", \"target\":\"{}\", \"expr\":{} }}",
        target, expr ? expr->to_string() : "null"
    );
}

unique_ptr<Block> Block::parse(Lexer::Stream& s) {
    std::vector<unique_ptr<NodeBase>> nodes;

    s.expect(Lexer::Type::BraceOpen);
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

    Span span = s.peek()->span;
    s.expect(Lexer::Type::BraceClose);

    return std::make_unique<Block>(std::move(nodes), span);
}

string Block::to_string() {
    string out = "{ \"type\":\"Block\", \"nodes\":[";

    for(auto& n : nodes) {
        out += n->to_string();
        out += ",";
    }

    out.pop_back();
    out += "]}";

    return out;
}

unique_ptr<Return> Return::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    
    s.expect("return", span);
    unique_ptr<Expr> expr = Expr::parse(s);

    span.end = expr->span_meta.end;
    return std::make_unique<Return>(std::move(expr), span);
}

string Return::to_string() { return std::format("{{ \"type\":\"Return\", \"expr\":{} }}", expr->to_string()); }

unique_ptr<Include> Include::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;

    s.expect("include");
    
    s.expect(Lexer::Type::ParenOpen);
    string lang = s.peek()->content;
    s.expect(Lexer::Type::StringLiteral);
    s.expect(Lexer::Type::Comma);
    string type = s.peek()->content;
    s.expect(Lexer::Type::StringLiteral);
    s.expect(Lexer::Type::Comma);
    string file = s.peek()->content;
    s.expect(Lexer::Type::StringLiteral);
    s.expect(Lexer::Type::ParenClose);

    return std::make_unique<Include>(lang, type, file, span);
}

std::unique_ptr<Do> Do::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    s.expect("do");
    auto block = NodeBase::parse(s);
    s.expect("while");
    auto cond = Expr::parse(s);
    
    return std::make_unique<Do>(std::move(block), std::move(cond), span);
}

std::unique_ptr<While> While::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    s.expect("while");
    auto cond = Expr::parse(s);
    auto block = NodeBase::parse(s);

    return std::make_unique<While>(std::move(block), std::move(cond), span);
}

std::unique_ptr<Mod> Mod::parse(Lexer::Stream& s) {
    Span span = s.peek()->span;
    s.expect("mod");
    span.end = s.peek()->span.end;
    string name = s.pop()->content;

    return std::make_unique<Mod>(name, span);
}