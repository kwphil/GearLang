#pragma once

#include <variant>
#include <optional>

#include "type.hpp"

using std::optional;
using std::variant;

namespace Sem {
    struct ExprValue {
        bool is_const;
        Type ty;
    };

    struct Variable {
        std::string name;
        Type type;
        // 0 is false, 1 is true, 2 is need to be checked
        uint8_t is_global;
        std::unique_ptr<Ast::Nodes::NodeBase>* let_stmt;
    };
}