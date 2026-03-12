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

#include <unordered_set>
#include <algorithm>
#include <format>
#include <cctype>
#include <tuple>

#include <gearlang/lex.hpp>

using std::tuple;
using std::unordered_set;
using std::string;

extern unordered_set<string> keywords;
extern unordered_set<string> operators;

Lexer::Type Lexer::classify(std::string& content, CharType state) {
    switch(state) {
        case CharType::Alpha:
            return keywords.contains(content)
                ? Type::Keyword
                : Type::Identifier;

        case CharType::Paren:
            if(content == "(") return Type::ParenOpen;
            if(content == ")") return Type::ParenClose;
            break;

        case CharType::Brace:
            if(content == "{") return Type::BraceOpen;
            if(content == "}") return Type::BraceClose;
            break;

        case CharType::Num: {
            if(content == "...")
                return Type::Ellipsis;

            bool has_alpha =
                std::any_of(content.begin(), content.end(),
                [](char c){
                    return std::isalpha(
                        static_cast<unsigned char>(c));
                });

            if(has_alpha)
                return Type::Identifier;

            return content.contains('.')
                ? Type::FloatLiteral
                : Type::IntegerLiteral;
        }

        case CharType::Sym:
            if(content == ",") return Type::Comma;
            if(content == "=") return Type::Equal;

            if(operators.contains(content))
                return Type::Operator;

            return Type::Invalid;

        case CharType::Quote:
            return Type::StringLiteral;

        case CharType::Caret:
            return Type::Caret;

        case CharType::Semi:
            return Type::Semi;

        case CharType::At:
            return Type::At;

        case CharType::Hash:
            return Type::Hash;

        default:
            break;
    }

    throw std::runtime_error(std::format(
        "Unknown lexer state: {}", (int)state
    ));
}
