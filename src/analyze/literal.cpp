#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>
#include <gearlang/sem/val.hpp>

using namespace Ast::Nodes;
using Sem::ExprValue;
using Sem::Type;

ExprValue* ExprLitInt::analyze(Sem::Analyzer& analyzer) {
    if(value <= 0xff) ty = new Type("i8");
    else if(value <= 0xffff) ty = new Type("i16");
    else if(value <= 0xffffffff) ty = new Type("i32");
    else ty = new Type("i64");

    return new ExprValue(true, *ty);
}

ExprValue* ExprLitFloat::analyze(Sem::Analyzer& analyzer) {
    ty = new Type("f32");
    return new ExprValue(true, *ty);
}

ExprValue* ExprLitString::analyze(Sem::Analyzer& analyzer) {
    ty = new Type("char^");
    return new ExprValue(true, *ty);
}