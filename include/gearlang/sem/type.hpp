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

#define GEARLANG_TYPE_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <llvm/IR/Type.h>

#include "../ctx.hpp"
#include "../lex.hpp"

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::optional;
using std::unordered_map;

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
            I64,
            U8,
            U16,
            U32,
            U64,
            F32,
            F64,
            Invalid
        };

        using Struct = vector<pair<string, shared_ptr<Type>>>; 
    private:
        static unordered_map<string, shared_ptr<Struct>> struct_list;
        static unordered_map<string, shared_ptr<Struct>> union_list;

        PrimType prim_type = PrimType::Invalid;
        shared_ptr<Struct> record_type;
        string record_name;
        bool record_is_struct; // is false if union 
        unsigned int pointer = 0;

        shared_ptr<Type> array_type;
        unsigned int array_size = -1;

        static PrimType parse_primitive(std::string& s);
        static PrimType parse_primitive(Lexer::Stream& s);

        /// @brief Parses the underlying type into an llvm type. 
        /// @param ctx the global context (GearLang)
        /// @returns the llvm type
        inline llvm::Type* underlying_to_llvm(Context& ctx) const {
            if(record_type) return get_llvm_struct();
            return primitive_to_llvm(prim_type, ctx);
        }
    public:
        Type() : prim_type(PrimType::Invalid) { };
        Type(Lexer::Stream& s);
        Type(
            PrimType prim_type, 
            int pointer, 
            shared_ptr<Struct> struct_type = { }, 
            string struct_name = ""
        )
        = delete;
        /// @brief Builds the type with a string
        /// @param s the string
        explicit inline Type(const char* s) { *this = Type((std::string)s); } 
        inline Type(std::string s) {
            Lexer::Stream stream = Lexer::tokenize_by_string(s);
            *this = Type(stream);
        }
        /// @brief Builds the type from a list of pairs of strings and Types, creating a struct
        /// @param s the struct object
        /// @param name the name of the struct
        Type(Struct s, string name="unnamed_struct")
        : record_type(std::make_shared<Struct>(s)) 
        { struct_list.insert({ name, record_type }); }
        
        bool operator==(Type&& other) {
            return 
                record_type == other.record_type &&
                record_is_struct == other.record_is_struct &&
                array_type == other.array_type && 
                prim_type == other.prim_type &&
                pointer == other.pointer;
        }

        bool operator==(const char* other) {
            return *this == Type(other);
        }

        /// @brief Checks if the type is a primitive (no pointer)
        bool is_primitive() const { return !(pointer || record_type || array_type); }
        /// @brief Checks if the type is a primitive (does not account for pointers)
        bool is_underlying_primitive() const { return !(record_type || array_type); }
        /// @brief Checks if the type is a struct type (does not account for pointers)
        bool is_struct() const { return record_type.get() && record_is_struct; }
        /// @brief Checks if the type is a union type (does not account for pointers)
        bool is_union() const { return record_type.get() && !record_is_struct; }
        /// @brief Checks if the type is an array type (does not account for pointers)
        bool is_array() const { return array_type.get(); }
        /// @brief Checks if the type is a pointer
        bool is_pointer_ty() const;
        /// @brief How many pointers stacked on top of eachother. T^ would return 1, T^^ = 2, ...
        int pointer_level() const;
        /// @brief Wraps a pointer type around the current type and returns it
        Type ref();
        /// @brief Unwraps a pointer type
        Type deref();
        /// @brief Wraps an array around the type and returns it
        Type array();
        /// @brief Wraps an array around the type and returns it
        /// @param size The size of the array
        Type array(int size);
        /// @brief Checks if the type is an fxx type
        bool is_float() const;
        /// @brief Checks if the type is an ixx type
        bool is_int() const;
        /// @brief The 8-bit type (int -> i8, float -> f16). Assumes this is a primitive
        PrimType bits_low_type() const;
        /// @brief Checks the bit level (8bits=0, 16=1, 32=2, ...). Assumes this is a primitive
        inline int bit_level() const { return (int)prim_type-(int)bits_low_type(); }
        /// @brief Gets the index of a variable in a struct by name.
        /// @param name the name of the parameter
        /// @returns -1 if the the parameter doesn't exist, otherwise the index of the parameter
        /// @throws std::logic_error if type is not a struct 
        int struct_parameter_index(string name);

        /// @brief Converts the type to an llvm Type
        /// @param ctx The global context (GearLang context, not llvm)
        /// @return The returning type
        llvm::Type* to_llvm(Context& ctx) const;
        /// @brief If the type is a pointer OR array, grabs the underlying type (returns nullptr if not)
        /// @param ctx The global context (GearLang context, not llvm)
        /// @return The returning type
        llvm::Type* get_underlying_type(Context& ctx) const;

        /// @brief Checks the type of a parameter at a given index.
        /// @param index The index
        inline Type struct_param_ty(int index) {
            return *record_type->at(index).second;
        }

        /// @brief Takes a primitive and converts it directly to an llvm type
        /// @param ty the type to convert
        /// @param ctx the global context (GearLang context, not llvm)
        /// @return the returning type
        static llvm::Type* primitive_to_llvm(PrimType ty, Context& ctx);

        /// @brief Converts a Struct type into an llvm type. Use this when defining a struct
        /// @param obj The struct to convert
        /// @param ctx the global context (GearLang, not llvm)
        /// @return the returning type
        static llvm::Type* struct_to_llvm(Type& obj, Context& ctx, string name);

        /// @brief Get an already translated struct
        static llvm::Type* get_llvm_struct(string name, Struct& obj);
        llvm::Type* get_llvm_struct() const;

        /// @brief String representation of the type
        std::string dump();

        /// @brief Checks if another type is compatible with this one
        bool is_compatible(Type& other) { return is_compatible(static_cast<Type&&>(other)); }
        bool is_compatible(Type&& other);
    };
}
