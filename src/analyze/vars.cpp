#include <optional>

#include <gearlang/sem/analyze.hpp>
#include <gearlang/sem/val.hpp>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>

#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using namespace Sem;

#include <iostream>

void Let::analyze(Analyzer& analyzer) {
    ExprValue* rvalue;
    Type ty;

    ty = Type("void"); // TODO: Walk through the program and find the type if no rvalue is provided

    if(expr.has_value()) {
        ExprValue* rvalue = expr.value()->analyze(analyzer);
        std::cout << *expr << std::endl;
        ty = expr.value()->get_type().value(); 
    }

    Variable var = {
        .name=target,
        .type=ty,
        .is_global=static_cast<uint8_t>(analyzer.is_global_scope()*2),
        .let_stmt=this
    };

    analyzer.add_variable(target, var);
}

ExprValue* ExprVar::analyze(Analyzer& analyzer) {
    optional<Variable> var_wrap = analyzer.decl_lookup(name);

    if(!var_wrap.has_value()) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = var_wrap.value();

    let = var.let_stmt;
    ty = new Type(var.type);

    return new ExprValue {
        .is_const=false,
        .ty=var.type
    };
}

ExprValue* ExprAssign::analyze(Analyzer& analyzer) {
    optional<Variable> var_wrap = analyzer.decl_lookup(name);

    if(!var_wrap.has_value()) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = var_wrap.value();

    let = var.let_stmt;
    ty = new Type(var.type);

    return expr->analyze(analyzer);
}

ExprValue* Argument::analyze(Analyzer& analyzer) {
    Variable var = {
        .name=name,
        .type=*ty,
        .is_global=false,
        .let_stmt=this
    };

    analyzer.add_variable(name, var);

    return nullptr;
}