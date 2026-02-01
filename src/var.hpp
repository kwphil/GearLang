#pragma once

#include <string>
#include <llvm/IR/Type.h>
#include "ctx.hpp"

namespace Ast {
    enum class Type {
        Void,
        I32,
        F32,
        Invalid,
    };

    llvm::Type* type_to_llvm_type(Type ty, Context& ctx);

    struct Variable {
        std::string name;
        Type ty;
    };
}
