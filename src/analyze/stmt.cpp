#include <gearlang/ast/stmt.hpp>
#include <gearlang/sem/analyze.hpp>

#include <iostream>

using namespace Ast::Nodes;
using namespace Sem;

void Return::analyze(Analyzer& analyzer) {
    expr->analyze(analyzer);
}