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
#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

#include <format>

using namespace Sem;

unordered_map<string, shared_ptr<Type::Struct>> Type::struct_list = { };

/* ============================
   Queries
   ============================ */

bool Type::is_pointer_ty() const {
    return pointer;
}

int Type::pointer_level() const {
    if(!is_pointer_ty()) return -1;

    return pointer;
}

std::string Type::dump() {
    std::string s;

    if(is_struct()) {
        for(auto& entry : struct_list) {
            if(entry.second == struct_type) {
                s = entry.first;
                goto pointer_dump;
            }
        }

        s = "invalid";
        goto pointer_dump;
    }

    using enum PrimType;
    switch(prim_type) {
        case(Void): s = "void"; break;
        case(Bool): s = "bool"; break;
        case(Char): s = "char"; break;
        case(I8): s = "i8"; break;
        case(I16): s = "i16"; break;
        case(I32): s = "i32"; break;
        case(F32): s = "f32"; break;
        case(F64): s = "f64"; break;
        default: s = "invalid"; break; 
    }

    if(is_primitive()) return s;

pointer_dump:
    if(!is_pointer_ty()) return s;
    for(unsigned int i = 1; i < pointer; i++)
        s.push_back('^');

    return s;
}

bool Type::is_float() const {
    if(!is_primitive()) return false;

    using enum PrimType;
    switch(prim_type) {
        case(F32):
        case(F64): return true;
        default: return false;
    }
}

bool Type::is_int() const {
    if(!is_primitive()) return false;

    if(prim_type >= PrimType::Char && prim_type <= PrimType::I32) return true;
    return false;
}

bool Type::is_compatible(Type&& other) {
    if(is_pointer_ty() && other.is_pointer_ty()) return true;
    if(is_float() && other.is_float()) return true;
    if(is_int() && other.is_int()) return true;
    if(struct_type == other.struct_type) return true;

    return false;
}

int Type::struct_parameter_index(string name) {
    if(!is_struct()) {
        throw std::logic_error("Expected a struct type");
    }

    for(unsigned int i = 0; i < struct_type->size(); i++) {
        pair<string, Type>& curr = struct_type->at(i);
        if(name == curr.first) {
            return i;
        }
    }

    return -1;
}

Type::PrimType Type::bits_low_type() const {
    if(prim_type == PrimType::Void) return PrimType::Void;
    if(prim_type == PrimType::Char) return PrimType::I8;

    if(is_int()) return PrimType::I8;
    if(is_float()) return PrimType::F32;
    
    return PrimType::Invalid;
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
