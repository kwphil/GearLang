#include <gearlang/sem/analyze.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/ast/stmt.hpp>

using namespace Ast::Nodes;
using Sem::Analyzer;

void Let::analyze(Analyzer& analyzer) {
    Sem::ExprValue* rvalue = expr->analyze(analyzer);

    Sem::Variable var = {
        .name=target,
        .type=rvalue->ty,
        .is_global=analyzer.is_global_scope()*2,
    };

    analyzer.add_variable(target, var);
}