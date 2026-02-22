#pragma once
#include <memory>

#include "../ctx.hpp"
#include "../lex.hpp"

#include "base.hpp"
#include "expr.hpp"

namespace Ast::Nodes {
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
        : expr(std::move(expr)), cond(std::move(cond)), Stmt(line_number) { }

        If(If&&) = default;
        If& operator=(If&&) = default;
        If(const If&) = delete;

        static std::unique_ptr<If> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override { }
        void generate(Context& ctx) override;
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
        ) : else_expr(std::move(expr)), If(std::move(if_expr)) { }

        static std::unique_ptr<Else> parse(
            std::unique_ptr<If>,
            Lexer::Stream& s
        );

        virtual void analyze(Sem::Analyzer& analyzer) override { }
        void generate(Context& ctx);
    };

    /// @brief Node for variable declarations
    class Let : public Stmt {
    private:
        /// @brief The target variable name
        std::string target;
        /// @brief The expression for the variable's initial value
        pExpr expr;

    public:
        bool is_global = false;

        Let(std::string& target, pExpr expr, int line_number)
        : target(target), expr(std::move(expr)), Stmt(line_number) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        std::string get_name() { return target; }
        void generate(Context& ctx) override;
        void analyze(Sem::Analyzer& analyzer) override; 
    };

    /// @brief Node for return statements
    class Return : public Stmt {
    private:
        /// @brief The expression to return
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr, int line_number)
        : expr(std::move(expr)), Stmt(line_number) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override { }
        void generate(Context& ctx) override;
    };

    /// @brief Node for function definitions
    class Function : public Stmt {
    private:
        /// @brief The function name
        std::string name;
        /// @brief The function return type
        Sem::Type ty;
        /// @brief The function arguments
        std::vector<Sem::Variable> args;
        /// @brief The function body block
        std::unique_ptr<NodeBase> block;
        /// @brief If the function is variadic
        bool is_variadic;

    public:
        Function(
            std::string& name, 
            Sem::Type ty, 
            std::vector<Sem::Variable> args, 
            std::unique_ptr<NodeBase> block, 
            bool is_variadic,
            int line_number
        ) : 
            name(name), ty(ty), args(args), is_variadic(is_variadic),
            block(std::move(block)), Stmt(line_number) { } 

        static std::unique_ptr<Function> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override { }
        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };

    class ExternFn : public Stmt {
    private:
        /// @brief the callee name
        std::string callee;
        /// @brief the function return type
        Sem::Type ty;
        /// @brief args
        std::vector<Sem::Variable> args;
        /// @brief is_variadic
        bool is_variadic;
        /// @brief not implemented yet, but forces no name mangling
        bool no_mangle;

    public:
        ExternFn(
            std::string& callee, 
            Sem::Type ty,
            std::vector<Sem::Variable>& args, 
            bool is_variadic,
            bool no_mangle,
            int line_number
        ) : callee(callee), args(args), ty(ty), 
            is_variadic(is_variadic), no_mangle(no_mangle),
            Stmt(line_number) { }

        static std::unique_ptr<ExternFn> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override { }
        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };
}
