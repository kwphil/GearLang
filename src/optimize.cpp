#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/lit.hpp>
#include <gearlang/ast/vars.hpp>
#include <gearlang/optimizer.hpp>
#include <gearlang/etc.hpp>

#include <deque>

using namespace Ast::Nodes;
using std::move;
using std::deque;
using std::unique_ptr;
using std::make_unique;

unique_ptr<NodeBase> Ast::optimize(unique_ptr<NodeBase> node) {
    if(cast_to<Expr>(node.get())) {
        return optimize_expr(unique_ptr<Expr>(cast_to<Expr>(node.release())));
    }

    return move(node);
}

unique_ptr<Expr> Ast::optimize_expr(unique_ptr<Expr> node) {
    if(cast_to<ExprOp>(node.get())) {
        auto cast = unique_ptr<ExprOp>(cast_to<ExprOp>(node.get()));
        return Optimizer::fold(move(cast));
    }

    return move(node);
}

unique_ptr<Expr> Optimizer::fold(unique_ptr<ExprOp> expr) {
    bool left = is_const(expr->left.get());
    bool right = is_const(expr->right.get());
    
    // Try to find other ExprOp's
    auto try_fold = [&](unique_ptr<Expr> expr, bool pass) -> unique_ptr<Expr> {
        if (!pass) return move(expr);
            
        return Ast::optimize_expr(move(expr));
    };

    expr->left = move(try_fold(move(expr->left), left));
    expr->right = move(try_fold(move(expr->right), right));

    // Then see if we can fold this statement
    if(!(left && right)) {
        auto node = std::make_unique<ExprOp>(
            expr->type, 
            std::move(expr->left), 
            std::move(expr->right), 
            expr->span_meta
        );
        
        node->set_type(*expr->get_type());
        return move(node);
    }

    if(auto l = cast_to<ExprLitFloat>(expr->left.get())) {
        auto r = cast_to<ExprLitFloat>(expr->right.get());

        auto new_obj = std::make_unique<ExprLitFloat>(l->value + r->value, expr->span_meta);
        new_obj->set_type(*expr->get_type());
        return move(new_obj);
    }

    auto l = cast_to<ExprLitInt>(expr->left.get());
    auto r = cast_to<ExprLitInt>(expr->right.get());

    auto new_obj = std::make_unique<ExprLitInt>(l->value + r->value, expr->span_meta);
    new_obj->set_type(*expr->get_type());
    return move(new_obj);
}

bool Optimizer::is_const(Expr* expr) {
    if(auto op = cast_to<ExprOp>(expr))
        return is_const(op->left.get()) && is_const(op->right.get());
    
    if(cast_to<ExprLitInt>(expr) || cast_to<ExprLitFloat>(expr))
        return true;
    
    return false;
}
