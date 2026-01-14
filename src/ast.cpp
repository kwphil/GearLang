#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <format>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>

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

    std::string error = "Unexpected ExprOp: ";
    error += type;
    throw std::runtime_error(error);
}

llvm::Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);

    switch (type) {
        case Add: return ctx.builder.CreateAdd(lhs, rhs, "addtmp");
        case Sub: return ctx.builder.CreateSub(lhs, rhs, "subtmp");
        case Mul: return ctx.builder.CreateMul(lhs, rhs, "multmp");
        case Div: return ctx.builder.CreateSDiv(lhs, rhs, "divtmp");
    }

    throw std::runtime_error("Invalid ExprOp");
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

std::unique_ptr<Ast::Nodes::ExprVar> Ast::Nodes::ExprVar::parse(std::string& name) { 
    return std::make_unique<ExprVar>(name);
}

std::string Ast::Nodes::ExprVar::show() { return name; }

llvm::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    auto it = ctx.named_values.find(name);
    if (it == ctx.named_values.end())
        throw std::runtime_error("Unknown variable: " + name);

    return ctx.builder.CreateLoad(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        it->second,
        name + ".load"
    );
}

// ExprAssign

std::unique_ptr<Ast::Nodes::ExprAssign>
Ast::Nodes::ExprAssign::parse(std::string& name, Lexer::Stream& s) {
    s.expect("=");
    pExpr expr = Expr::parse(s);

    return std::make_unique<ExprAssign>(name, std::move(expr));
}

std::string Ast::Nodes::ExprAssign::show() {
    return name + " = " + expr->show();
}

llvm::Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    llvm::Value* alloca = ctx.named_values[name];
    if (!alloca)
        throw std::runtime_error("Unknown variable: " + name);

    llvm::Value* value = expr->generate(ctx);
    ctx.builder.CreateStore(value, alloca);

    return value;
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
        default: break;
    }

    s.pop();
    
    if(lit.type == Lexer::Type::Identifier) {
        if(lit.content == "if") {
            s.back();
            return If::parse(s);
        }

        if(s.peek().content == "=") {
            return ExprAssign::parse(lit.content, s);
        }

        return ExprVar::parse(lit.content);
    }

    std::string error_msg = std::format("Unexpect token: {} (type={})", lit.content, (int)lit.type);
    throw std::runtime_error(error_msg);
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

    if (s.peek().content == "let")         out = Let::parse(s);
    else if (s.peek().content == "return") out = Return::parse(s);
    else if (s.peek().content == "if")     out = If::parse(s);
    else                                   out = Expr::parse(s);
    s.expect(";");

    return out;
}

// Return -------------------------------------------------

std::unique_ptr<Ast::Nodes::Return> Ast::Nodes::Return::parse(Lexer::Stream& s) {
    s.expect("return");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Return>(Return(std::move(expr)));
}

std::string Ast::Nodes::Return::show() {
    return "return " + expr->show() + ";";
}

llvm::Value* Ast::Nodes::Return::generate(Context& ctx) {
    llvm::Value* retVal = expr->generate(ctx);

    // Ensure i32
    if (retVal->getType()->isIntegerTy() &&
        retVal->getType()->getIntegerBitWidth() < 32) {
        retVal = ctx.builder.CreateSExt(
            retVal,
            llvm::Type::getInt32Ty(ctx.llvmCtx)
        );
    }

    // Linux x86-64: exit(int status)
    // rdi = status
    // rax = 60
    auto* asmFn = llvm::InlineAsm::get(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(ctx.llvmCtx),
            { llvm::Type::getInt32Ty(ctx.llvmCtx) },
            false
        ),
        "mov $$60, %rax\n"
        "syscall",
        "{rdi},~{rax},~{rcx},~{r11},~{memory}",
        true
    );

    ctx.builder.CreateCall(asmFn, { retVal });
    ctx.builder.CreateUnreachable();
    return retVal;
}

// If --------------------------------------------

std::unique_ptr<Ast::Nodes::If> Ast::Nodes::If::parse(Lexer::Stream& s) {
    s.expect("if");
    pExpr cond = Expr::parse(s);
    pExpr expr = Expr::parse(s);

    return std::make_unique<If>(If(std::move(expr), std::move(cond)));
}

std::string Ast::Nodes::If::show() {
    return "if" + cond->show() + " --- " + expr->show();
}

llvm::Value* Ast::Nodes::If::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "then", fn);

    // Generate condition ONCE
    llvm::Value* condVal = cond->generate(ctx);

    // Convert to boolean: cond != 0
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(ctx.llvmCtx),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, then_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    expr->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // continuation
    ctx.builder.SetInsertPoint(then_block);

    return nullptr;
}

// Program ---------------------------

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has())
        that.content.push_back(Nodes::NodeBase::parse(s));
        // std::cout << s.peek().content << std::endl;

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
