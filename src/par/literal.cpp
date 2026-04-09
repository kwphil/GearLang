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

#include <gearlang/ast/lit.hpp>
#include <gearlang/lex.hpp>

#include <memory>
#include <string>
#include <format>

using std::string;
using std::unique_ptr;
using namespace Ast::Nodes;

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

unique_ptr<ExprLitChar> ExprLitChar::parse(Lexer::Stream& s) {
    auto t = s.pop();

    return std::make_unique<ExprLitChar>(t->content[0], t->span);
}

string ExprLitChar::to_string() {
    return std::format("{{ \"type\":\"ExprLitChar\", \"char\":\"{}\" }}", c);
}