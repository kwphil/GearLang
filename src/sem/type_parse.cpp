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
   Construction
   ============================ */

Type::Type(Lexer::Stream& s) {
    auto tok = s.peek();
    pointer = 0;

    if(s.peek()->content == "struct") {
        string name = "unnamed_struct";
        s.pop();

        if(s.peek()->type == Lexer::Type::Identifier) name=s.pop()->content;

        s.expect("{");

        struct_type = std::make_shared<Struct>();

        while(s.peek()->type != Lexer::Type::BraceClose) {
            pair<string, Type> arg;

            arg.first = s.pop()->content;
            arg.second = Type(s);

            s.expect(";");

            struct_type->push_back(arg);
        }

        s.pop(); // }
        
        struct_list.insert({ name, struct_type });
        return;
    }

    // First see if there's a defined struct
    auto _struct = struct_list.find(s.peek()->content);
    if(_struct != struct_list.end()) {
        struct_type = _struct->second;
        s.pop();
        goto pointer_parse;
    }

    prim_type = parse_primitive(s);

pointer_parse:
    if (s.peek()->type == Lexer::Type::Caret) {
        while (s.peek()->type == Lexer::Type::Caret) {
            s.pop();
            pointer++;
        }

        return;
    }

    if (prim_type == PrimType::Invalid && !struct_type) {
        Error::throw_error(
            tok->span,
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

    Type new_type = *this;
    new_type.pointer -= 1;
    return new_type;
}
