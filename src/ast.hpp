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

        static std::unique_ptr<ExprVar> parse(Lexer::Stream& s);

        std::string show() override;
        llvm::Value* generate(Context& ctx) override ;
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
