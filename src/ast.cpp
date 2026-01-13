#include <stdexcept>
#include <vector>
#include <memory>
#include <iostream>

#include <llvm/IR/Value.h>

#include "ast.hpp"
#include "lex.hpp"
#include "ctx.hpp"

// Expr -------------------------------------------------------

std::unique_ptr<Ast::Nodes::Expr> Ast::Nodes::Expr::parse(Lexer::Stream& s) {
    return parseExpr(s);
}

// ExprOp -----------------------------------------------------

std::string Ast::Nodes::ExprOp::show() {
    switch (type) {
        case Add: return "(" + left->show() + " + " + right->show() + ")"; break;
        case Sub: return "(" + left->show() + " - " + right->show() + ")"; break;
        case Mul: return "(" + left->show() + " * " + right->show() + ")"; break;
        case Div: return "(" + left->show() + " / " + right->show() + ")"; break;
    }
}

llvm::Value* Ast::Nodes::ExprOp::generate(Context& ctx) { 
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);

    switch(type) {
        case Add:
            return ctx.builder.CreateAdd(lhs, rhs, "addtmp");

        case Sub:
            return ctx.builder.CreateSub(lhs, rhs, "subtmp");

        case Mul:
            return ctx.builder.CreateMul(lhs, rhs, "multmp");

        case Div:
            return ctx.builder.CreateSDiv(lhs, rhs, "divtmp");

    }

    throw std::runtime_error("Unexpected ExprOp: " + type);
}

// ExprLitInt ---------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitInt> Ast::Nodes::ExprLitInt::parse(Lexer::Stream& s) 
{ return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop().content)); }

std::string Ast::Nodes::ExprLitInt::show() 
{ return std::to_string(this->val); } 

llvm::Value* Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        this->val,
        true
    );
}

// ExprLitFloat ------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitFloat> Ast::Nodes::ExprLitFloat::parse(Lexer::Stream& s) 
{ return std::make_unique<Ast::Nodes::ExprLitFloat>(std::stod(s.pop().content)); }

std::string Ast::Nodes::ExprLitFloat::show() { return std::to_string(val); }

llvm::Value* Ast::Nodes::ExprLitFloat::generate(Context& ctx) {}; //TODO

// ExprVar --------------------------------------------------

std::unique_ptr<Ast::Nodes::ExprVar> Ast::Nodes::ExprVar::parse(Lexer::Stream& s) { 
    return std::make_unique<ExprVar>(s.pop().content);
}

std::string Ast::Nodes::ExprVar::show() { return name; }

llvm::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    llvm::Value* alloca = ctx.named_values[name];

    if(!alloca)
        throw std::runtime_error("Unknown variable: " + name);

    return ctx.builder.CreateLoad(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        alloca,
        name
    );
}

// Expr ------------------------------------------------

Ast::Nodes::pExpr Ast::Nodes::Expr::parseExpr(Lexer::Stream& s) {
    pExpr left = parseTerm(s);
    
    //no operator
    if (s.peek().type != Lexer::Type::Operator)
        return left;

    
    ExprOp::Type type;
    switch(s.pop().content[0])
    {
        case '+': type = ExprOp::Type::Add; break;
        case '-': type = ExprOp::Type::Sub; break;
        case '*': type = ExprOp::Type::Mul; break;
        case '/': type = ExprOp::Type::Div; break;
    }

    pExpr right = parseTerm(s);
    return std::make_unique<ExprOp>(type, std::move(left), std::move(right));
}

Ast::Nodes::pExpr Ast::Nodes::Expr::parseTerm(Lexer::Stream& s) {
    if (s.peek().content == "(") {
        s.expect("(");
        auto expr = parseExpr(s);
        s.expect(")");
        return expr;
    }

    Lexer::Token lit = s.peek();

    switch (lit.type) {
        case Lexer::Type::FloatLiteral:   return ExprLitFloat::parse(s); break;
        case Lexer::Type::IntegerLiteral: return ExprLitInt::parse(s);   break;
        case Lexer::Type::Identifier:     return ExprVar::parse(s);      break;
    }
}

// Let -------------------------------------------------

std::unique_ptr<Ast::Nodes::Let> Ast::Nodes::Let::parse(Lexer::Stream& s) {
    s.expect("let");
    std::string target = s.pop().content;
    s.expect("=");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Let>(target, std::move(expr));
}

std::string Ast::Nodes::Let::show() {
    return "let " + target + " = " + expr->show() + ";";
}

llvm::Value* Ast::Nodes::Let::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::Type* ty = llvm::Type::getInt32Ty(ctx.llvmCtx);
    llvm::AllocaInst* alloca = ctx.create_entry_block(fn, target, ty);

    // TODO: make this an optional parameter
    if(true) {
        llvm::Value* initVal = expr->generate(ctx);
        ctx.builder.CreateStore(initVal, alloca);
    }

    ctx.named_values[target] = alloca;
    return alloca;
}

std::unique_ptr<Ast::Nodes::NodeBase> Ast::Nodes::NodeBase::parse(Lexer::Stream& s) {
    std::unique_ptr<NodeBase> out;

    if (s.peek().content == "let") out = Let::parse(s);
    else                           out = Expr::parse(s);
    s.expect(";");

    return out;
}

// Program ---------------------------

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has())
        that.content.push_back(Nodes::NodeBase::parse(s));

    return that;
}

void Ast::Program::show(std::ostream& os) {
    for (auto& node : content) {
        os << node->show() << std::endl;
    }
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& expr : content)
        expr->generate(ctx);
}
