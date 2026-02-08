#include <memory>

#include "../ctx.hpp"
#include "../lex.hpp"

namespace Ast::Nodes {
#ifndef NODE_BASE
#define NODE_BASE
    /// @brief Base class for all AST nodes
    class NodeBase {
    public:
        /// @brief The line number in the source code where the node appears
        const int line_number;

        NodeBase(int line_number) : line_number(line_number) {}
        virtual ~NodeBase() = default;
        /// @brief Parses a node from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed node
        static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);
    };
}
#endif
#ifndef EXPR_BASE

#include "expr.hpp"

#endif
namespace Ast::Nodes {
    /// @brief Base class for statements (i.e. functions, ifs and others)
    class Stmt {
    public:
        /// @brief generate function that doesn't return anything
        /// @param ctx the context
        virtual void generate(Context& ctx) = 0;
    };

    /// @brief Node for if statements
    class If : public NodeBase {
    protected:
        /// @brief The condition expression
        pExpr cond;
        /// @brief The expression to execute if the condition is true
        std::unique_ptr<NodeBase> expr;

    public:
        If(std::unique_ptr<NodeBase> expr, pExpr cond, int line_number)
        : expr(std::move(expr)), cond(std::move(cond)), NodeBase(line_number) { }

        If(If&&) = default;
        If& operator=(If&&) = default;
        If(const If&) = delete;

        static std::unique_ptr<If> parse(Lexer::Stream& s);

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
        void generate(Context& ctx);
    };

    /// @brief Node for variable declarations
    class Let : public NodeBase {
    private:
        /// @brief The target variable name
        std::string target;
        /// @brief The expression for the variable's initial value
        pExpr expr;

    public:
        Let(std::string& target, pExpr expr, int line_number)
        : target(target), expr(std::move(expr)), NodeBase(line_number) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        void generate(Context& ctx) override;
    };

    /// @brief Node for return statements
    class Return : public NodeBase {
    private:
        /// @brief The expression to return
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr, int line_number)
        : expr(std::move(expr)), NodeBase(line_number) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        void generate(Context& ctx) override;
    };

    /// @brief Node for function definitions
    class Function : public NodeBase {
    private:
        /// @brief The function name
        std::string name;
        /// @brief The function return type
        Ast::Type ty;
        /// @brief The function arguments
        std::vector<Ast::Variable> args;
        /// @brief The function body block
        std::unique_ptr<NodeBase> block;
        /// @brief If the function is variadic
        bool is_variadic;

    public:
        Function(
            std::string& name, 
            Ast::Type ty, 
            std::vector<Ast::Variable> args, 
            std::unique_ptr<NodeBase> block, 
            bool is_variadic,
            int line_number
        ) : 
            name(name), ty(ty), args(args), is_variadic(is_variadic),
            block(std::move(block)), NodeBase(line_number) { } 

        static std::unique_ptr<Function> parse(Lexer::Stream& s);

        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };

    class ExternFn : public NodeBase {
    private:
        /// @brief the callee name
        std::string callee;
        /// @brief the function return type
        Ast::Type ty;
        /// @brief args
        std::vector<Ast::Variable> args;
        /// @brief is_variadic
        bool is_variadic;

    public:
        ExternFn(
            std::string& callee, 
            Ast::Type ty,
            std::vector<Ast::Variable>& args, 
            bool is_variadic,
            int line_number
        ) : callee(callee), args(args), ty(ty), 
            is_variadic(is_variadic), NodeBase(line_number) { }

        static std::unique_ptr<ExternFn> parse(Lexer::Stream& s);

        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };
}
