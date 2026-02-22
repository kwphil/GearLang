#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/analyze.hpp>

using namespace Ast::Nodes;
using Sem::Analyzer;
using Sem::ExprValue;

#define FOLD_OP(op) \
    if(is_float) float_val = lhs->float_val op rhs->float_val; \
    else int_val = lhs->int_val op rhs->int_val;

#include <iostream>

ExprValue* ExprOp::analyze(Analyzer& analyzer) {
    ExprValue* lhs = left->analyze(analyzer);
    ExprValue* rhs = right->analyze(analyzer);

    analyzer.type_is_compatible(lhs->ty, rhs->ty);

    bool is_float = lhs->ty.is_float();

    // Checking if we can fold the operation
    if(lhs->is_const && rhs->is_const) {
        union {
            uint64_t int_val;
            double float_val;
        };

        // Sorry for the shitty code to follow
        // I'm just lazy :p
        switch(type) {
            case(Add): FOLD_OP(+); break;
            case(Sub): FOLD_OP(-); break;
            case(Mul): FOLD_OP(*); break;
            case(Div): FOLD_OP(/); break;
        }

        std::cout << "Folded statement into value: " << int_val << std::endl;

        return new ExprValue(int_val, lhs->ty);
    }

    std::cout << "Could not fold statement" << std::endl;

    return new ExprValue(lhs, rhs, lhs->ty);
}