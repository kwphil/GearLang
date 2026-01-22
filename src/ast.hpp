#pragma once

#include <memory>
#include <string>

#include <llvm/IR/Value.h>

#include "ctx.hpp"
#include "lex.hpp"

namespace Ast::Nodes {
    class NodeBase {
    public:
        virtual ~NodeBase() = default;
        virtual std::string show() = 0;
        virtual llvm::Value* generate(Context& context) = 0;
        static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);
    };

    class Expr : public NodeBase {
    public:
        virtual ~Expr() = default;

        static std::unique_ptr<Expr> parse(Lexer::Stream& s);

        static std::unique_ptr<Expr> parseExpr(Lexer::Stream& s);
        static std::unique_ptr<Expr> parseTerm(Lexer::Stream& s);
    };

    using pExpr = std::unique_ptr<Expr>;

    class ExprOp : public Expr {
    public:
        enum Type { Add, Sub, Mul, Div } type;
        std::unique_ptr<Expr> left, right;

        ExprOp(Type type, pExpr left, pExpr right)
        : type(type), left(std::move(left)), right(std::move(right)) {};

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    class ExprLitInt : public Expr {
    private:
        uint64_t val;
        
    public:
        ExprLitInt(uint64_t x) : val(x) {}
        static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s);
        std::string show() override;

        virtual llvm::Value* generate(Context& ctx) override;
    };

    class ExprLitFloat : public Expr {
        double val;

    public:
        ExprLitFloat(double x) : val(x) {}
        static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s);
        std::string show() override;
        
        llvm::Value* generate(Context& ctx) override;
    };

    class ExprVar : public Expr {
    private:
        const std::string name;

    public:
        ExprVar(const std::string& name)
        : name(name) {};

        static std::unique_ptr<ExprVar> parse(std::string& name);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    class ExprAssign : public Expr {
    private:
        const std::string name;
        pExpr expr;

    public:
        ExprAssign(const std::string& name, pExpr expr)
        : name(name), expr(std::move(expr)) { }

        static std::unique_ptr<ExprAssign> parse(std::string& name, Lexer::Stream& s);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    // Rust-style block (returns a value)
    class ExprBlock : public Expr {
    private:
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

    class If : public Expr {
    private:
        pExpr cond;
        pExpr expr;

    public:
        If(pExpr expr, pExpr cond)
        : expr(std::move(expr)), cond(std::move(cond)) { }

        static std::unique_ptr<If> parse(Lexer::Stream& s);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override;
    };

    class Let : public NodeBase {
    private:
        std::string target;
        pExpr expr;

    public:
        Let(std::string& target, pExpr expr) 
        : target(target), expr(std::move(expr)) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    class Return : public NodeBase {
    private:
        // std::shared_ptr<llvm::Function*> parent_fn; // TODO
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr)
        : expr(std::move(expr)) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        std::string show() override;

        llvm::Value* generate(Context& ctx) override;
    };

    class Function : public NodeBase {
    private:
        std::string name;
        // TODO: Add args
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
    class Program {
    private:
        std::vector<std::unique_ptr<Nodes::NodeBase>> content;
       
    public: 
        static Program parse(Lexer::Stream& s);

        void show(std::ostream& os);

        void generate(Context& ctx);
    };
};
