#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/etc.hpp>

#include <iostream>

using namespace Ast::Nodes;
using namespace Sem;

void If::analyze(Analyzer& analyzer) {
    analyze_nodebase(&expr, analyzer);
}

void Else::analyze(Analyzer& analyzer) {
    cond->analyze(analyzer);
    analyze_nodebase(&expr, analyzer);
    analyze_nodebase(&else_expr, analyzer);
}

void Return::analyze(Analyzer& analyzer) {
    expr->analyze(analyzer);
}

void Function::analyze(Analyzer& analyzer) {
    Analyzer::Scope* fn_scope = analyzer.new_scope();

    for(auto& arg : args) { // Won't matter too much to change values 
        arg->analyze(analyzer);
    }
    
    analyze_nodebase(&block, analyzer);

    analyzer.delete_scope();

    Sem::Variable var = {
        .name=name,
        .type=ty,
        .is_global=1
    };

    analyzer.add_variable(name, var);
}

void ExternFn::analyze(Analyzer& analyzer) {
    Sem::Variable var = {
        .name=callee,
        .type=ty,
        .is_global=1
    };

    analyzer.add_variable(callee, var);
}