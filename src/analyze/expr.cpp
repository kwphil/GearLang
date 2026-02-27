#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/error.hpp>

#include <optional>
#include <format>

using namespace Ast::Nodes;
using namespace Sem;
using std::optional;

unique_ptr<ExprValue> ExprCall::analyze(Analyzer& analyzer) {
    optional<Variable> ref = analyzer.decl_lookup(callee);

    if(!ref.has_value()) {
        Error::throw_error(
            line_number,
            callee.c_str(),
            "Function not defined",
            Error::ErrorCodes::FUNCTION_NOT_DEFINED
        );
    }

    Variable retval = ref.value();

    for(auto& arg : args) {
        arg->analyze(analyzer);
    } 

    return std::make_unique<ExprValue>(false, retval.type);
}

unique_ptr<ExprValue> ExprBlock::analyze(Analyzer& analyzer) {
    analyzer.new_scope();
    
    for(auto& node : nodes) {
        analyze_nodebase(&node, analyzer);
    }

    analyzer.delete_scope();

    return nullptr;
}

unique_ptr<ExprValue> ExprAddress::analyze(Analyzer& analyzer) {
    optional<Variable> lookup = analyzer.decl_lookup(name);

    if(!lookup.has_value()) {
        Error::throw_error(
            line_number,
            std::format("#{}", name).c_str(),
            "Variable not defined",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = lookup.value();

    ty = std::make_unique<Type>(var.type.ref());
    let = var.let_stmt;

    return std::make_unique<ExprValue>(false, *ty);
}

unique_ptr<ExprValue> ExprDeref::analyze(Analyzer& analyzer) {
    optional<Variable> lookup = analyzer.decl_lookup(name);

    if(!lookup.has_value()) {
        Error::throw_error(
            line_number,
            std::format("@{}", name).c_str(),
            "Variable not defined",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = lookup.value();
    ty = std::make_unique<Type>(var.type.deref());
    let = var.let_stmt;

    return std::make_unique<ExprValue>(false, var.type);
}