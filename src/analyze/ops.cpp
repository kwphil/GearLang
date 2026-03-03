#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/analyze.hpp>

using namespace Ast::Nodes;
using namespace Sem;

unique_ptr<ExprValue> ExprOp::analyze(Analyzer& analyzer) {
    unique_ptr<ExprValue> lhs = left->analyze(analyzer);
    unique_ptr<ExprValue> rhs = right->analyze(analyzer);

    if(!analyzer.type_is_compatible(lhs->ty, rhs->ty)) {
        throw std::runtime_error("Bad type");
    }

    // Checking for boolean operators
    if(type >= Type::Gt) {
        ty = std::make_unique<Sem::Type>("bool");
    } else {
        ty = std::make_unique<Sem::Type>(lhs->ty);
    }

    return std::make_unique<ExprValue>(lhs->is_const && rhs->is_const, lhs->ty);
}