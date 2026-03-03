#pragma once

#include <string>
#include <vector>
#include <optional>

#include <llvm/IR/Type.h>

#include "../ctx.hpp"
#include "../lex.hpp"

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

        /// @brief Non primitive types (WARNING: DO NOT USE DIRECTLY)
        using NonPrimitive = std::vector<int>;

    private:
        PrimType prim_type = PrimType::Void;
        std::optional<NonPrimitive> non_prim;

        static PrimType parse_primitive(std::string& s);
        static PrimType parse_primitive(Lexer::Stream& s);
        static NonPrimitive parse_nonprimitive(Lexer::Stream& s, PrimType prim_type);

    public:
        Type() = default;
        Type(Lexer::Stream& s);
        Type(PrimType prim_type, NonPrimitive non_prim) 
        : prim_type(prim_type), non_prim(non_prim) { } 
        /// @brief Builds the type with a constant string
        /// @param s the string
        constexpr explicit Type(const char* s); 

        /// @brief Checks if the type is a primitive
        bool is_primitive() const { return !non_prim.has_value(); }
        /// @brief Checks if the type is a pointer
        bool is_pointer_ty() const;
        /// @brief How many pointers stacked on top of eachother. T& would return 1, T&& = 2, ...
        int pointer_level() const;
        /// @brief Wraps a pointer type around the current type and returns it
        Type ref();
        /// @brief Unwraps a pointer type
        Type deref();
        /// @brief Checks if the type is an fxx type
        bool is_float() const;

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

        std::string dump();
    };

    // Define constexpr
    constexpr Type::Type(const char* s) {
        std::string str = s;

        if(str.back() == '^') {
            int count = 1;
            int i;

            for(i = str.size()-1; i >= 0; i--) {
                if(str[i] != '^') break;

                count++;
            }

            std::string prim_str = str.substr(0, str.size()-1);
            prim_type = parse_primitive(prim_str);

            non_prim = {
                { 0, count, static_cast<int>(prim_type) }
            };

            return;
        }

        prim_type = parse_primitive(str);

        if(prim_type == PrimType::Invalid) {
            throw std::runtime_error("Invalid type");
        }
    }
}