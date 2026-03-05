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

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <llvm/IR/Type.h>

#include "../ctx.hpp"
#include "../lex.hpp"

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;

namespace Sem {
    class Type {
    public:
        /// @brief Primitive types
        enum class PrimType {
            Void,
            Bool,
            Char,
            I8,
            I16,
            I32,
            F32,
            F64,
            Invalid
        };

        using Struct = vector<pair<string, Type>>; 
    private:
        static vector<shared_ptr<Struct>> struct_list;

        PrimType prim_type = PrimType::Invalid;
        shared_ptr<Struct> struct_type;
        unsigned int pointer;

        static PrimType parse_primitive(std::string& s);
        static PrimType parse_primitive(Lexer::Stream& s);

    public:
        Type() = default;
        Type(Lexer::Stream& s);
        Type(PrimType prim_type, int pointer) 
        : prim_type(prim_type), pointer(pointer) { } 
        /// @brief Builds the type with a constant string
        /// @param s the string
        constexpr explicit Type(const char* s);
        /// @brief Builds the type from a list of pairs of strings and Types, creating a struct
        Type(Struct s)
        : struct_type(std::make_shared<Struct>(s)) 
        { struct_list.push_back(struct_type); }
        
        /// @brief Checks if the type is a primitive (no pointer)
        bool is_primitive() const { return !(pointer || struct_type); }
        /// @brief Checks if the type is a primitive (does not account for pointers)
        bool is_underlying_primitive() const { return !struct_type; }
        /// @brief Checks if the type is a struct type (does not account for pointers)
        bool is_struct() const { return struct_type.get(); }
        /// @brief Checks if the type is a pointer
        bool is_pointer_ty() const;
        /// @brief How many pointers stacked on top of eachother. T^ would return 1, T^^ = 2, ...
        int pointer_level() const;
        /// @brief Wraps a pointer type around the current type and returns it
        Type ref();
        /// @brief Unwraps a pointer type
        Type deref();
        /// @brief Checks if the type is an fxx type
        bool is_float() const;
        /// @brief Checks if the type is an ixx type
        bool is_int() const;

        /// @brief Converts the type to an llvm Type
        /// @param ctx The global context (GearLang context, not llvm)
        /// @return The returning type
        llvm::Type* to_llvm(Context& ctx) const;
        /// @brief If the type is a pointer, grabs the underlying type (returns nullptr if not)
        /// @param ctx The global context (GearLang context, not llvm)
        /// @return The returning type
        llvm::Type* get_underlying_type(Context& ctx) const;

        /// @brief Takes a primitive and converts it directly to an llvm type
        /// @param ty the type to convert
        /// @param ctx the global context (GearLang context, not llvm)
        /// @return the returning type
        static llvm::Type* primitive_to_llvm(PrimType ty, Context& ctx);

        /// @brief String representation of the type
        std::string dump();

        /// @brief Checks if another type is compatible with this one
        bool is_compatible(Type& other) { return is_compatible(static_cast<Type&&>(other)); }
        bool is_compatible(Type&& other);
    };

    constexpr Type::Type(const char* s) {
        std::string str = s;

        pointer = 0;
        if(str.back() == '^') {
            int count = 1;
            int i;

            for(i = str.size()-1; i >= 0; i--) {
                if(str[i] != '^') break;

                count++;
            }

            std::string prim_str = str.substr(0, str.size()-1);
            prim_type = parse_primitive(prim_str);

            pointer = count;

            return;
        }

        prim_type = parse_primitive(str);

        if(prim_type == PrimType::Invalid) {
            throw std::runtime_error("Invalid type");
        }
    }
}
