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

unique_ptr<If> If::parse(Lexer::Stream& s) {
    Span span = s.peek()->span; // Get it here on the line of the if statement itself
    s.expect("if", span);
    pExpr cond = Expr::parse(s);
    unique_ptr<NodeBase> expr = NodeBase::parse(s);

    span.end = expr->span_meta.end;
    return std::make_unique<If>(std::move(expr), std::move(cond), span);
}

string If::to_string() {
    return std::format("{{ \"type\":\"If\", \"cond\"={}, \"expr\"={} }}", cond->to_string(), expr->to_string());
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
    return std::format("{{ \"type\":\"Else\", \"cond\":{}, \"exprtrue\":{}, \"exprfalse\":{} }}",
        cond->to_string(), expr->to_string(), else_expr->to_string()
    );
}