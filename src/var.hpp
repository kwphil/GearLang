#pragma once

#include <string>
#include <llvm/IR/Type.h>
#include "ctx.hpp"
#include "lex.hpp"

namespace Ast {
    enum class Type {
        Void,
        Char,
        I32,
        F32,
        F64,
        Pointer,
        Invalid,
        NonPrimitive,
    };

    // Might add other things later
    struct NonPrimitive {
        std::vector<int> type;
    };

    Ast::Type parse_type(std::string& s);
    Ast::Type parse_type(Lexer::Stream& s);
    Ast::NonPrimitive parse_nonprim(Lexer::Stream& s);

    llvm::Type* type_to_llvm_type(Type ty, Context& ctx);
    llvm::Type* type_to_llvm_type(NonPrimitive ty, Context& ctx);

    struct Variable {
        std::string name;
        Type ty;
        NonPrimitive npty;
    };
}
