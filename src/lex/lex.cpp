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

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

#include <iostream>
#include <fstream>
#include <format>

Lexer::CharType Lexer::getCharType(char c);

bool Lexer::Stream::has() {
    return index != content.size();
}

std::unique_ptr<Lexer::Token> Lexer::Stream::peek() {
    if(index >= content.size()) return nullptr;
    return std::make_unique<Lexer::Token>(content[index]);
}

std::unique_ptr<Lexer::Token> Lexer::Stream::next() {
    if(index + 1 >= content.size()) return nullptr;
    return std::make_unique<Lexer::Token>(content[index+1]);
}

std::unique_ptr<Lexer::Token> Lexer::Stream::pop() {
    return std::make_unique<Lexer::Token>(content[index++]);
}

void Lexer::Stream::expect(
    const char* should,
    Span const& span
) {
    auto is = pop()->content;
    if (is != should) {
        Error::throw_error(
            span,
            std::format(
                "Parser found an error. Expected `{}` but received `{}`",
                should, is
            ).c_str(),
            Error::ErrorCodes::EXPECT_VALUE
        );
    }
}

void Lexer::Stream::expect(Type should, Span const& span) {
    auto is = pop()->type;
    if (is != should) {
        Error::throw_error(
            span,
            std::format(
                "Parser found an error. Expected `{}` but received `{}`",
                print_type(should), print_type(is)
            ).c_str(),
            Error::ErrorCodes::EXPECT_VALUE
        );
    }
}

void Lexer::Stream::dump() {
    for (size_t i = index; i < content.size(); i++)
        std::cout << content[i].content << " ";
    std::cout << "\n";
}

std::string Lexer::Stream::to_string() {
    std::string out = "[\n";
    
    for(auto& tok : content) {
        std::string token_type = print_type(tok.type);
        // Making sure escape characters get rendered as regular text
        std::string content;
        for(auto& c : tok.content) {
            if(c == '\n') {
                content.push_back('\\');
                content.push_back('n');
                continue;
            }

            if(c == '\t') {
                content.push_back('\\');
                content.push_back('t');
                continue;
            }

            content.push_back(c);
        }

        std::string curr = std::format("[\"{}\", {}, \"{}\"],\n", 
            content, 
            tok.span.line, 
            token_type
        );

        out += curr;
    }

    out.pop_back();
    out.pop_back();
    return out + "\n]";
}

const char* Lexer::print_type(Lexer::Type ty) {
    using namespace Lexer; // Just to clean a little

    switch(ty) {
        case(Type::BraceClose): return "BraceClose";
        case(Type::BraceOpen): return "BraceOpen";
        case(Type::ParenClose): return "ParenClose";
        case(Type::ParenOpen): return "ParenOpen";
        case(Type::FloatLiteral): return "FloatLiteral";
        case(Type::IntegerLiteral): return "IntegerLiteral";
        case(Type::StringLiteral): return "StringLiteral";
        case(Type::Keyword): return "Keyword";
        case(Type::Identifier): return "Identifier";
        case(Type::Operator): return "Operator";
        case(Type::Ellipsis): return "Ellipsis";
        case(Type::Comma): return "Comma";
        case(Type::Semi): return "Semi";
        case(Type::Caret): return "Caret";
        case(Type::Hash): return "Hash";
        case(Type::Equal): return "Equal";
        case(Type::At): return "At";
        default: return "Invalid";
    }
}
