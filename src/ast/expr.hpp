#pragma once

#include <memory>

#include "../lex.hpp"
#include "../ctx.hpp"
#include "../sem/type.hpp"

#include "base.hpp"

namespace Ast::Nodes {
    /// @brief Base class for all nodes that return a value
    class Expr : public NodeBase {
    public:
        Expr(int line_number) : NodeBase(line_number) {}
        virtual ~Expr() = default;

        static std::unique_ptr<Expr> parse(Lexer::Stream& s);

        /// @brief Parses an expression from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed expression
        static std::unique_ptr<Expr> parseExpr(Lexer::Stream& s);
        /// @brief Parses a term from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed term
        static std::unique_ptr<Expr> parseTerm(Lexer::Stream& s);
        /// @brief Generates the llvm code
        virtual Value* generate(Context& ctx) = 0;
    };

    /// @brief Smart pointer type for expressions
    using pExpr = std::unique_ptr<Expr>;

    /// @brief Expression node for binary operations
    class ExprOp : public Expr {
    public:
        /// @brief Type of binary operation
        enum Type { Add, Sub, Mul, Div } type;
        /// @brief operands
        std::unique_ptr<Expr> left, right;

        ExprOp(Type type, pExpr left, pExpr right, int line_number)
        : type(type), left(std::move(left)), right(std::move(right)), Expr(line_number) {};

        Value* generate(Context& ctx) override;
    };

    /// @brief Template base class for literal expressions
    class Literal : public Expr {
    protected:
        llvm::Type* cast_type;
        
    public:
        Literal(int line_number, llvm::Type* cast) 
        : cast_type(cast), Expr(line_number) { }
        
        virtual ~Literal() = default;
    
        static std::unique_ptr<Literal> parse(Lexer::Stream& s, llvm::Type* cast = nullptr);
    };

    /// @brief Expression node for integer literals
    class ExprLitInt : public Literal {
    private:
        uint64_t value;

    public:
        ExprLitInt(uint64_t x, int line_number) : Literal(line_number, nullptr), value(x) {}
        static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s);

        virtual Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for floating-point literals
    class ExprLitFloat : public Literal {
    private:
        double value;

    public:
        ExprLitFloat(double x, int line_number) : Literal(line_number, nullptr), value(x) { }
        static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s);
        
        virtual Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for C-strings 
    class ExprLitString : public Literal {
    private:
        std::string string;

    public:
        ExprLitString(std::string& s, int line_number) : Literal(line_number, nullptr), string(s) { }
        static std::unique_ptr<ExprLitString> parse(Lexer::Stream& s);

        virtual Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for variable references
    class ExprVar : public Expr {
    private:
        /// @brief The variable name
        const std::string name;

    public:
        ExprVar(const std::string& name, int line_number)
        : name(name), Expr(line_number) {};

        static std::unique_ptr<ExprVar> parse(const Lexer::Token& name);
        Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for variable assignments
    class ExprAssign : public Expr {
    private:
        /// @brief The variable name
        const std::string name;
        /// @brief The expression to assign to the variable
        pExpr expr;

    public:
        ExprAssign(const std::string& name, pExpr expr, int line_number)
        : name(name), expr(std::move(expr)), Expr(line_number) { }

        static std::unique_ptr<ExprAssign> parse(const Lexer::Token& name, Lexer::Stream& s);

        Value* generate(Context& ctx) override;
    };

    class ExprCall : public Expr {
    private:
        /// @brief the arguments
        std::vector<pExpr> args;
        /// @brief the name of the function to call
        std::string callee;
    
    public:
        ExprCall(
            std::string& callee, 
            std::vector<pExpr>& args, 
            int line_number
        ) 
        : callee(callee), args(std::move(args)), Expr(line_number) { }

        static std::unique_ptr<ExprCall> parse(
            const Lexer::Token& name,
            Lexer::Stream& s
        );

        Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for referencing variables
    class ExprAddress : public Expr {
    private:
        /// @brief the name of the variable to reference
        std::string name;

    public:
        ExprAddress(std::string& name, int line_number)
        : name(name), Expr(line_number) { }

        static std::unique_ptr<ExprAddress> parse(Lexer::Stream& s);

        Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for the variable to dereference
    class ExprDeref : public Expr {
    private:
        /// @brief the name of the variable to dereference
        std::string name;

    public:
        ExprDeref(std::string& name, int line_number)
        : name(name), Expr(line_number) { }

        static std::unique_ptr<ExprDeref> parse(Lexer::Stream& s);

        Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for blocks of nodes
    class ExprBlock : public Expr {
    private:
        /// @brief The list of nodes in the block
        std::vector<std::unique_ptr<NodeBase>> nodes;
    
    public:
        ExprBlock(std::vector<std::unique_ptr<NodeBase>>&& nodes, int line_number)
        : Expr(line_number), nodes(std::move(nodes)) { }

        static std::unique_ptr<ExprBlock> parse(Lexer::Stream& s);

        // Will probably return the return variable
        // Will stay void until I get Function to return non-void
        Value* generate(Context& ctx) override;
    };
}
