#pragma once

#include <string>

#include <llvm/IR/Type.h>

#include "ctx.hpp"
#include "lex.hpp"

namespace Ast {
    class Type {
    public:
        enum class PrimType {
            Void,
            Char,
            I32,
            F32,
            F64,
            Invalid,
            NonPrimitive
        };

        struct NonPrimitive {
            std::vector<int> type;
        };

    private:
        /// @brief the primitive type. Returns NonPrimitve when not primitive
        PrimType prim_type;

        /// @brief the optional non primitive value. 
        NonPrimitive* non_prim;

        // PRIVATE FUNCTIONS
        PrimType parse_type(std::string& s);
        PrimType parse_type(Lexer::Stream& s);
        NonPrimitive* parse_nonprim(Lexer::Stream& s);

        llvm::Type* type_to_llvm_type(Context& ctx);
        llvm::Type* type_to_llvm_type_np(Context& ctx);

    public:
        /// @brief constructor to build the type
        /// @param s the lexer stream
        Type(Lexer::Stream& s);

        /// @brief default constructor
        Type()
        : prim_type(PrimType::Void), non_prim(nullptr) { }

        /// @brief Call primtype parse directly
        static llvm::Type* type_to_llvm_type(PrimType ty, Context& ctx);

        /// @brief the llvm generator
        llvm::Type* generate(Context& ctx);
    };

    struct Variable {
        std::string name;
        Type ty;
    };
}
