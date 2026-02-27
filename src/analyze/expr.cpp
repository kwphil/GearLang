#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/error.hpp>

#include <optional>
#include <format>

using namespace Ast::Nodes;
using namespace Sem;
using std::optional;

ExprValue* ExprCall::analyze(Analyzer& analyzer) {
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

    return new ExprValue {
        .is_const=false,
        .ty=retval.type
    };
}

ExprValue* ExprBlock::analyze(Analyzer& analyzer) {
    analyzer.new_scope();
    
    for(auto& node : nodes) {
        analyze_nodebase(&node, analyzer);
    }

    analyzer.delete_scope();

    return nullptr;
}

ExprValue* ExprAddress::analyze(Analyzer& analyzer) {
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

    ty = new Type(var.type.ref());

    return new ExprValue {
        .is_const=false,
        .ty=*ty
    };
}

ExprValue* ExprDeref::analyze(Analyzer& analyzer) {
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

    return new ExprValue {
        .is_const=false,
        .ty=var.type
    };
}