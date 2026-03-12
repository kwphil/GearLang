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
#include <memory>

#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::unique_ptr;
using std::string;
using std::vector;

vector<string> Ast::keyword_list = { }; 

unique_ptr<NodeBase> NodeBase::parse(Lexer::Stream& s) {
    if(!s.has()) {
        Error::throw_error(
            Span{},
            "Unexpected EOF",
            Error::ErrorCodes::UNEXPECTED_EOF
        );
    }

    auto curr = s.peek();
    const string& tok = curr->content;

    if(tok == "export") {
        keyword_list.push_back(s.pop()->content);
        return parse(s);
    }

    // no semicolon statements
    if(tok == "fn") return Function::parse(s);
    if(tok == "{")  return ExprBlock::parse(s);

    if(tok == "if") {
        auto if_expr = If::parse(s);

        if(s.peek()->content == "else")
            return Else::parse(std::move(if_expr), s);

        return if_expr;
    }

    // semicolon statements
    unique_ptr<NodeBase> out;

    if(tok == "let")        out = Let::parse(s);
    else if(tok == "return")out = Return::parse(s);
    else if(tok == "extern")out = ExternFn::parse(s);
    else if(tok == "struct")out = Struct::parse(s);
    else                    out = Expr::parse(s);

    s.expect(";", s.peek()->span);

    return out;
}

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program program;

    while(s.has()) {
        keyword_list.clear();
        program.content.push_back(NodeBase::parse(s));
    }

    return program;
}

string Ast::Program::to_string() {
    string out = "[\n";

    for(size_t i = 0; i < content.size(); i++) {
        out += content[i]->to_string();

        if(i + 1 != content.size())
            out += ",\n";
    }

    out += "\n]";

    return out;
}
