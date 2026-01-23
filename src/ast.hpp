#pragma once

#include <memory>
#include <string>

#include <llvm/IR/Value.h>

#include "ctx.hpp"
#include "lex.hpp"

namespace Ast::Nodes {
    /// @brief Base class for all AST nodes
    class NodeBase {
    public:
        virtual ~NodeBase() = default;
        /// @brief Returns a string representation of the node
        /// @return A string representation of the node
        virtual std::string show() = 0;
        /// @brief Generates LLVM IR for the node
        /// @param context The context to generate the IR in
        /// @return The generated LLVM value
        virtual llvm::Value* generate(Context& context) = 0;
        /// @brief Parses a node from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed node
        static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);
    };

    class Expr : public NodeBase {
    public:
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

        ExprOp(Type type, pExpr left, pExpr right)
        : type(type), left(std::move(left)), right(std::move(right)) {};

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for integer literals
    class ExprLitInt : public Expr {
    private:
        /// @brief The integer value
        uint64_t val;
        
    public:
        ExprLitInt(uint64_t x) : val(x) {}
        static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s);
        std::string show() override;

        virtual llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for floating-point literals
    class ExprLitFloat : public Expr {
    private:
        /// @brief The floating-point value
        double val;

    public:
        ExprLitFloat(double x) : val(x) {}
        static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s);
        std::string show() override;
        
        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for variable references
    class ExprVar : public Expr {
    private:
        /// @brief The variable name
        const std::string name;

    public:
        ExprVar(const std::string& name)
        : name(name) {};

        static std::unique_ptr<ExprVar> parse(std::string& name);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for variable assignments
    class ExprAssign : public Expr {
    private:
        /// @brief The variable name
        const std::string name;
        /// @brief The expression to assign to the variable
        pExpr expr;

    public:
        ExprAssign(const std::string& name, pExpr expr)
        : name(name), expr(std::move(expr)) { }

        static std::unique_ptr<ExprAssign> parse(std::string& name, Lexer::Stream& s);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Expression node for conditional statements
    class ExprBlock : public Expr {
    private:
        /// @brief The list of nodes in the block
        std::vector<std::unique_ptr<NodeBase>> nodes;
    
    public:
        ExprBlock(std::vector<std::unique_ptr<NodeBase>>&& nodes)
        : nodes(std::move(nodes)) { }

        static std::unique_ptr<ExprBlock> parse(Lexer::Stream& s);

        std::string show() override;
        // Will probably return the return variable
        // Will stay void until I get Function to return non-void
        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Node for if statements
    class If : public Expr {
    private:
        /// @brief The condition expression
        pExpr cond;
        /// @brief The expression to execute if the condition is true
        std::unique_ptr<NodeBase> expr;

    public:
        If(std::unique_ptr<NodeBase> expr, pExpr cond)
        : expr(std::move(expr)), cond(std::move(cond)) { }

        static std::unique_ptr<If> parse(Lexer::Stream& s);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Node for variable declarations
    class Let : public NodeBase {
    private:
        /// @brief The target variable name
        std::string target;
        /// @brief The expression for the variable's initial value
        pExpr expr;

    public:
        Let(std::string& target, pExpr expr) 
        : target(target), expr(std::move(expr)) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Node for return statements
    class Return : public NodeBase {
    private:
        /// @brief The expression to return
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr)
        : expr(std::move(expr)) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    /// @brief Node for function definitions
    class Function : public NodeBase {
    private:
        /// @brief The function name
        std::string name;
        // TODO: Add args
        /// @brief The function body block
        std::unique_ptr<NodeBase> block;

    public:
        Function(std::string& name, std::unique_ptr<NodeBase> block)
        : name(name), block(std::move(block)) { } 

        static std::unique_ptr<Function> parse(Lexer::Stream& s);

        std::string show() override;

        // This has no use for generating code, so this always returns nullptr
        llvm::Value* generate(Context& ctx) override;
    };
};

namespace Ast {
    /// @brief The root AST node representing the entire program
    class Program {
    private:
        /// @brief The list of nodes in the program
        std::vector<std::unique_ptr<Nodes::NodeBase>> content;
       
    public: 
        static Program parse(Lexer::Stream& s);

        void show(std::ostream& os);

        void generate(Context& ctx);
    };
};
