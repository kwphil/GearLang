#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/lit.hpp>
#include <gearlang/optimizer.hpp>
#include <gearlang/etc.hpp>

using namespace Ast::Nodes;

void Optimizer::fold(ExprOp** _expr) {
    auto expr = *_expr;
    bool left = is_const(expr->left.get());
    bool right = is_const(expr->right.get());
    
    // Try to find other ExprOp's
    auto try_fold = [&](std::unique_ptr<Expr>& expr, bool pass) {
        if (!pass) return;
            
        if (auto op = cast_to<ExprOp>(expr.get())) {
            fold(&op);  
        }
    };

    try_fold(expr->left, left);
    try_fold(expr->right, right);

    // Then see if we can fold this statement
    if(!(left && right)) {
        auto node = std::make_unique<ExprOp>(
            expr->type, 
            std::move(expr->left), 
            std::move(expr->right), 
            expr->span_meta
        );
        
        node->set_type(*expr->get_type());
        expr = node.get();
    }

    if(auto l = cast_to<ExprLitFloat>(expr->left.get())) {
        auto r = cast_to<ExprLitFloat>(expr->right.get());

        auto new_obj = std::make_unique<ExprLitFloat>(l->value + r->value, expr->span_meta);
        new_obj->set_type(*expr->get_type());
        expr = reinterpret_cast<ExprOp*>(new_obj.get());
    }

    auto l = cast_to<ExprLitInt>(expr->left.get());
    auto r = cast_to<ExprLitInt>(expr->right.get());

    auto new_obj = std::make_unique<ExprLitInt>(l->value + r->value, expr->span_meta);
    new_obj->set_type(*expr->get_type());
    expr = reinterpret_cast<ExprOp*>(new_obj.get());
}

bool Optimizer::is_const(Expr* expr) {
    if(auto op = cast_to<ExprOp>(expr))
        return is_const(op->left.get()) && is_const(op->right.get());
    
    if(cast_to<ExprLitInt>(expr) || cast_to<ExprLitFloat>(expr))
        return true;
    
    return false;
}
