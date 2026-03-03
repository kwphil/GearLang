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

#include <gearlang/sem/type.hpp>
#include <gearlang/error.hpp>
#include <string>
#include <format>

using namespace Sem;
using std::string;

/* ============================
   Primitive parsing
   ============================ */

Type::PrimType Type::parse_primitive(string& s) {
    if (s == "void") return PrimType::Void;
    if (s == "bool") return PrimType::Bool;
    if (s == "char") return PrimType::Char;
    if (s == "i8")   return PrimType::I8;
    if (s == "i16")  return PrimType::I16;
    if (s == "i32")  return PrimType::I32;
    if (s == "f32")  return PrimType::F32;
    if (s == "f64")  return PrimType::F64;
    return PrimType::Invalid;
}

Type::PrimType Type::parse_primitive(Lexer::Stream& s) {
    return parse_primitive(s.pop()->content);
}

/* ============================
   Non-primitive parsing
   ============================ */

Type::NonPrimitive Type::parse_nonprimitive(Lexer::Stream& s, PrimType prim_type) {

    throw std::runtime_error(
        std::format("Unknown non-primitive type: {}", s.peek()->content));
}

/* ============================
   Construction
   ============================ */

Type::Type(Lexer::Stream& s) {
    auto tok = s.peek();

    prim_type = parse_primitive(s);
    pointer = 0;

    if (s.peek()->type == Lexer::Type::Caret) {
        while (s.peek()->type == Lexer::Type::Caret) {
            s.pop();
            pointer++;
        }

        return;
    }

    if (prim_type == PrimType::Invalid) {
        Error::throw_error(
            tok->span.line,
            tok->content.c_str(),
            "Unknown type",
            Error::ErrorCodes::UNKNOWN_TYPE
        );
    }
}

Type Type::ref() {
    return Type(std::format("{}^", dump()).c_str());
}

Type Type::deref() {
    assert(pointer != 0);

    return Type(prim_type, pointer-1);
}
