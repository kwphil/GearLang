#include <optional>

#include <gearlang/sem/analyze.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using namespace Sem;

void Let::analyze(Analyzer& analyzer) {
    ExprValue* rvalue = expr->analyze(analyzer);

    Variable var = {
        .name=target,
        .type=rvalue->ty,
        .is_global=static_cast<uint8_t>(analyzer.is_global_scope()*2),
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