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
            Char,
            I32,
            F32,
            F64,
            Invalid
        };

        /// @brief Non primitive types (WARNING: DO NOT USE DIRECTLY)
        struct NonPrimitive {
            std::vector<int> type;
        };

    private:
        PrimType prim_type = PrimType::Void;
        std::optional<NonPrimitive> non_prim;

        static PrimType parse_primitive(std::string& s);
        static PrimType parse_primitive(Lexer::Stream& s);
        static NonPrimitive parse_nonprimitive(Lexer::Stream& s, PrimType prim_type);

    public:
        /// @brief default constructor
        Type() = default;
        /// @brief Builds the type with a Lexer stream
        /// @param s the stream
        explicit Type(Lexer::Stream& s);

        /// @brief Checks if the type is a primitive
        bool is_primitive() const { return !non_prim.has_value(); }
        /// @brief Checks if the type is a pointer
        bool is_pointer_ty() const;
        /// @brief How many pointers stacked on top of eachother. T& would return 1, T&& = 2, ...
        int pointer_level() const;

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
    };

    struct Variable {
        std::string name;
        Type type;
        bool is_global = false;
    };
}