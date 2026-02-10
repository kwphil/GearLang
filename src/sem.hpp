#pragma once

#include <string>
#include <optional>

#include <llvm/IR/Type.h>

#include "ctx.hpp"
#include "lex.hpp"
#pragma once

#include <string>
#include <vector>
#include <optional>

#include <llvm/IR/Type.h>

#include "ctx.hpp"
#include "lex.hpp"

namespace Sem {
    class Type {
    public:
        enum class PrimType {
            Void,
            Char,
            I32,
            F32,
            F64,
            Invalid
        };

        struct NonPrimitive {
            std::vector<int> type;
        };

    private:
        PrimType prim_type = PrimType::Void;
        std::optional<NonPrimitive> non_prim;

        static PrimType parse_primitive(std::string& s);
        static PrimType parse_primitive(Lexer::Stream& s);
        static NonPrimitive parse_nonprimitive(Lexer::Stream& s);

    public:
        Type() = default;
        explicit Type(Lexer::Stream& s);

        bool is_primitive() const { return !non_prim.has_value(); }
        bool is_pointer_ty() const;

        llvm::Type* to_llvm(Context& ctx) const;
        llvm::Type* get_underlying_type(Context& ctx) const;

        static llvm::Type* primitive_to_llvm(PrimType ty, Context& ctx);
    };

    struct Variable {
        std::string name;
        Type type;
    };
}