#include <gearlang/ast/expr.hpp>
#include <gearlang/etc.hpp>

#include <memory>

using std::unique_ptr;
using namespace Ast::Nodes;

namespace Optimizer {
    /// @brief Attempts to fold an expression
    /// @param expr The exprop to fold and replace
    void fold(ExprOp** expr);
    /// @brief Checks if the expression is a constant
    bool is_const(Expr* expr);
};