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
#include <memory>

using namespace Sem;
using std::string;
using std::shared_ptr;
using std::make_shared;

Type::PrimType Type::parse_primitive(string& s) {
    if (s == "void") return PrimType::Void;
    if (s == "bool") return PrimType::Bool;
    if (s == "char") return PrimType::Char;
    if (s == "i8")   return PrimType::I8;
    if (s == "i16")  return PrimType::I16;
    if (s == "i32")  return PrimType::I32;
    if (s == "i64")  return PrimType::I64;
    if (s == "u8")   return PrimType::U8;
    if (s == "u16")  return PrimType::U16;
    if (s == "u32")  return PrimType::U32;
    if (s == "u64")  return PrimType::U64;
    if (s == "f32")  return PrimType::F32;
    if (s == "f64")  return PrimType::F64;
    return PrimType::Invalid;
}

Type::PrimType Type::parse_primitive(Lexer::Stream& s) {
    return parse_primitive(s.pop()->content);
}

/// @brief A counter to create unique anonymous record names
// Expect an example to be __GEAR_struct_anonymous_4 or __GEAR_union_anonymous_6
int anon_struct = 0;

/// @brief A cache for all unparsed types that might be filled by external files
// Since the AST is required to use a semantic type to properly parse what types are present, 
// we need a cache to fill in types that will then be parsed after the FFI step.
//
// We'll create a function to handle the parsing, 
// which will just parse each type, and assign them to each type that is associated with the type string
// 
// The span object is just for error handling, it points to what the FIRST instance of the type has to say
static unordered_map<string, pair<vector<Type*>, Span>> unparsed_types;

/// @brief Will stop from caching if there is an unparsable type
bool no_unparsed = false;

Type::Type(Lexer::Stream& s) {
    auto tok = s.peek();
    pointer = 0;

    if (!s.has()) {
        Error::throw_error(
            {0, 0, 0, 0},
            "Stream ended unexpectedly.",
            Error::ErrorCodes::INVALID_AST
        );
    }

    if(s.peek()->content == "array") {
        s.pop(); // array
        s.expect(Lexer::Type::ParenOpen);
        array_type = std::make_shared<Type>(s);

        if(s.peek()->type == Lexer::Type::Comma) {
            s.pop(); // ,
            array_size = std::stoi(s.pop()->content);
        }

        s.expect(Lexer::Type::ParenClose);

        return;
    }

    if(s.peek()->content == "union") {
        string name;
        s.pop();
        record_is_struct = false;

        if(s.peek()->type == Lexer::Type::Identifier) name = s.pop()->content;
        else {
            name = "__GEAR_union_anonymous_";
            name += std::to_string(anon_struct++);
        }

        if(s.peek()->type != Lexer::Type::BraceOpen) goto find_parse;

        record_type = new Struct;
        union_list.insert({ name, record_type });
        record_name = name;

        s.expect("{");

        while(s.peek()->type != Lexer::Type::BraceClose) {
            pair<string, shared_ptr<Type>> arg;

            arg.first = s.pop()->content;
            arg.second = make_shared<Type>(s);

            s.expect(";");

            record_type->push_back(arg);
        }

        s.expect("}");

        return;
    }

    if(s.peek()->content == "struct") {
        string name;
        s.pop();
        record_is_struct = true;

        if(s.peek()->type == Lexer::Type::Identifier) name=s.pop()->content;
        else {
            name = "__GEAR_struct_anonymous_";
            name += std::to_string(anon_struct++);
        }

        if(s.peek()->type != Lexer::Type::BraceOpen) goto find_parse;

        record_type = new Struct;
        struct_list.insert({ name, record_type });
        record_name = name;

        s.expect("{");

        while(s.peek()->type != Lexer::Type::BraceClose) {
            pair<string, shared_ptr<Type>> arg;

            arg.first = s.pop()->content;
            arg.second = make_shared<Type>(s);

            s.expect(";");

            record_type->push_back(arg);
        }

        s.expect("}");
    
        return;
    }

find_parse:
    // First see if there's a defined struct
    auto _struct = struct_list.find(s.peek()->content);
    auto _union = union_list.find(s.peek()->content);
    auto _alias = alias_list.find(s.peek()->content);
    if(_struct != struct_list.end()) {
        record_type = _struct->second;
        record_name = _struct->first;
        s.pop();
        goto pointer_parse;
    }

    if(_union != union_list.end()) {
        record_type = _union->second;
        record_name = _union->first;
        s.pop();
        goto pointer_parse;
    }

    if(_alias != alias_list.end()) {
        Type* new_type = _alias->second;

        prim_type = new_type->prim_type;
        record_type = new_type->record_type;
        record_name = new_type->record_name;
        record_is_struct = new_type->record_is_struct;
        pointer = new_type->pointer;
        array_type = new_type->array_type;
        array_size = new_type->array_size;
        goto pointer_parse;
    }

    prim_type = parse_primitive(s);

    if (prim_type == PrimType::Invalid) {
        if(no_unparsed) {
            is_invalid = true;
            return;
        }

        s.back();
        // If no type is found, we cache it and fill it in later
        if(!unparsed_types.contains(s.peek()->content)) {
            unparsed_types[s.pop()->content] = { { this }, s.peek()->span };
        } else {
            unparsed_types[s.pop()->content].first.push_back(this);
        }
    }
pointer_parse:
    if(!s.has()) return;
    if (s.peek()->type == Lexer::Type::Caret) {
        while (s.has() && s.peek()->type == Lexer::Type::Caret) {
            s.pop();
            pointer++;
        }

        return;
    }
}
#include <iostream>
void Type::parse_unparsed() {
    no_unparsed = true;

    for(auto& ty : unparsed_types) {
        Type new_ty = ty.first;

        if(new_ty.is_invalid) {
            // std::cout << "===============================\n";
            // for(auto& t : s.content) {
            //     std::cout << t.content << '\n';
            // }

            // std::cout << std::endl;

            Error::throw_error(
                ty.second.second,
                "Unknown type",
                Error::ErrorCodes::UNKNOWN_TYPE
            );
        }

        for(auto& curr : ty.second.first) {
            // Enforce that pointers stay the same
            unsigned int pointer_level = curr->pointer; 
            *curr = new_ty;
            curr->pointer = pointer_level;
        }
    }
}