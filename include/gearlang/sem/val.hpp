#pragma once

#include <variant>
#include <optional>

#include "type.hpp"

using std::optional;
using std::variant;

namespace Sem {
    struct ExprValue {
        bool is_const;
        union { 
            // When const
            union {
                uint64_t int_val;
                double float_val;
                const char* str_val;
            };

            // When non-const
            struct { ExprValue *lhs, *rhs; };
        };
        Type ty;

        // Non-const constructor
        ExprValue(ExprValue* l, ExprValue* r, Type t)
            : is_const(false), lhs(l), rhs(r), ty(t) {}

        // Const integer constructor
        ExprValue(uint64_t v, Type t)
            : is_const(true), int_val(v), ty(t) {}
    };

    // Just stores information that some objects might want to know
    struct InfoDump {
        char is_global;
        bool extern_linkage;
        bool is_expr;
        ExprValue* lhs;
        ExprValue* rhs;
        
    };

    struct Variable {
        std::string name;
        Type type;
        // 0 is false, 1 is true, 2 is need to be checked
        uint8_t is_global = false;
        uint node_index = -1;
    };
}