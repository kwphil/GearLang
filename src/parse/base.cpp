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

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::unique_ptr;
using std::string;

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
    else if (curr->content == "struct") out = Struct::parse(s);
    else                                out = Expr::parse(s);
    s.expect(";", s.peek()->span);

    return out;
}


Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has()) {
        that.content.push_back(Nodes::NodeBase::parse(s));
    }
    
    return that; 
}

std::string Ast::Program::to_string() {
    std::string out = "[";
    
    for(auto& n : content) {
        out += n->to_string();
        out += ",\n";
    }   

    out[out.size()-2] = ']';

    return out;
}
