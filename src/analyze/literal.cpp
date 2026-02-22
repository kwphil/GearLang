#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>
#include <gearlang/sem/val.hpp>

using namespace Ast::Nodes;
using Sem::ExprValue;
using Sem::Type;

ExprValue* ExprLitInt::analyze(Sem::Analyzer& analyzer) {
    Type ty;

    if(value <= 0xff) ty = Type("i8");
    else if(value <= 0xffff) ty = Type("i16");
    else if(value <= 0xffffffff) ty = Type("i32");
    else ty = Type("i64");

    return new ExprValue(value, ty);
}