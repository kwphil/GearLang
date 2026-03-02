#pragma once

#include <memory>
#include <optional>

#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>

#include "base.hpp"
#include "expr.hpp"

using std::optional;

namespace Sem {
    class Analyzer;
    struct ExprValue;
}

namespace Ast::Nodes {
    class Expr;
    using pExpr = std::unique_ptr<Expr>;

    /// @brief Base class for statements (i.e. functions, ifs and others)
    class Stmt : public NodeBase {
    public:
        Stmt(int line_number) : NodeBase(line_number) { }

        /// @brief For the semantic analyzer
        /// @param analyzer The analyzer object
        virtual void analyze(Sem::Analyzer& analyzer) = 0;

        /// @brief generate function that doesn't return anything
        /// @param ctx the context
        virtual void generate(Context& ctx) = 0;
    };

    /// @brief Node for if statements
    class If : public Stmt {
    protected:
        /// @brief The condition expression
        pExpr cond;
        /// @brief The expression to execute if the condition is true
        std::unique_ptr<NodeBase> expr;

    public:
        If(std::unique_ptr<NodeBase> expr, pExpr cond, int line_number)
        : Stmt(line_number), cond(std::move(cond)), expr(std::move(expr)) { }

        If(If&&) = default;
        If& operator=(If&&) = default;
        If(const If&) = delete;

        static std::unique_ptr<If> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx) override;

        virtual std::string to_string() override;
    };

    /// @brief If/Else
    class Else : public If {
    private:
        /// @brief The condition expression for false if
        std::unique_ptr<NodeBase> else_expr;

    public:
        Else(
            std::unique_ptr<NodeBase> expr,
            If&& if_expr
        ) : If(std::move(if_expr)), else_expr(std::move(expr)) { }

        static std::unique_ptr<Else> parse(
            std::unique_ptr<If>,
            Lexer::Stream& s
        );

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx);

        virtual std::string to_string() override;
    };

    /// @brief Node for variable declarations
    class Let : public Stmt {
    private:
        /// @brief The target variable name
        std::string target;
        /// @brief The expression for the variable's initial value
        optional<pExpr> expr;
    public:
        /// @brief The LLVM variable
        llvm::Value* var;

        /// @brief If the variable is to be generated as a global
        bool is_global = false;

        Let(std::string& target, optional<pExpr> expr, int line_number)
        : Stmt(line_number), target(target), expr(std::move(expr)) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        std::string get_name() { return target; }
        void generate(Context& ctx) override;
        void analyze(Sem::Analyzer& analyzer) override; 
        virtual std::string to_string() override;
    };

    /// @brief Node for return statements
    class Return : public Stmt {
    private:
        /// @brief The expression to return
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr, int line_number)
        : Stmt(line_number), expr(std::move(expr)) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx) override;
        virtual std::string to_string() override;
    };
}
