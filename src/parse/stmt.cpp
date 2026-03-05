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

using namespace Ast::Nodes;
using Sem::Type;
using std::unique_ptr;
using std::string;
using std::vector;
using std::pair;

unique_ptr<Struct> Struct::parse(Lexer::Stream& s) {
    s.expect("struct");
    std::string name = s.pop()->content;

    s.expect("{");
    
    vector<pair<string, Type>> args;
    while(s.peek()->type != Lexer::Type::BraceClose) {
        pair<string
    }

    s.pop();
}

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
        target, expr->to_string()
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
