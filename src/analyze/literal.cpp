#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>
#include <gearlang/sem/val.hpp>

using namespace Ast::Nodes;
using Sem::ExprValue;
using Sem::Type;

unique_ptr<ExprValue> ExprLitInt::analyze(Sem::Analyzer& analyzer) {
    Type raw_ty;

    if(value <= 0xff) raw_ty = Type("i8");
    else if(value <= 0xffff) raw_ty = Type("i16");
    else if(value <= 0xffffffff) raw_ty = Type("i32");
    else raw_ty = Type("i64");

    ty = std::make_unique<Type>(raw_ty);

    return std::make_unique<ExprValue>(true, *ty);
}

unique_ptr<ExprValue> ExprLitFloat::analyze(Sem::Analyzer& analyzer) {
    ty = std::make_unique<Type>("f32");
    return std::make_unique<ExprValue>(true, *ty);
}

unique_ptr<ExprValue> ExprLitString::analyze(Sem::Analyzer& analyzer) {
    ty = std::make_unique<Type>("char^");
    return std::make_unique<ExprValue>(true, *ty);
}