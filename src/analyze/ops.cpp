#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/analyze.hpp>

using namespace Ast::Nodes;
using Sem::Analyzer;
using Sem::ExprValue;

ExprValue* ExprOp::analyze(Analyzer& analyzer) {
    ExprValue* lhs = left->analyze(analyzer);
    ExprValue* rhs = right->analyze(analyzer);

    if(!analyzer.type_is_compatible(lhs->ty, rhs->ty)) {
        throw std::runtime_error("Bad type");
    }

    ty = new Sem::Type(lhs->ty);

    return new ExprValue(lhs->is_const && rhs->is_const, lhs->ty);
}