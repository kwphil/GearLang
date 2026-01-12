
#include <vector>
#include <memory>

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

void Ast::Nodes::ExprOp::generate(Context& ctx) { 
    right->generate(ctx);
    ctx.emit("push rax");
    left->generate(ctx);
    ctx.emit("pop rbx");
    
    switch (type) {
        case Add: ctx.emit("add rax, rbx"); break;
        case Sub: ctx.emit("sub rax, rbx"); break;
        case Mul: ctx.emit("imul rbx"); break;
        case Div: ctx.emit("idiv rbx"); break;
    }
}

// ExprLitInt ---------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitInt> Ast::Nodes::ExprLitInt::parse(Lexer::Stream& s) 
{ return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop().content)); }

std::string Ast::Nodes::ExprLitInt::show() 
{ return std::to_string(this->val); } 

void Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    ctx.emit("mov rax, " + std::to_string(val));
}

// ExprLitFloat ------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitFloat> Ast::Nodes::ExprLitFloat::parse(Lexer::Stream& s) 
{ return std::make_unique<Ast::Nodes::ExprLitFloat>(std::stod(s.pop().content)); }

std::string Ast::Nodes::ExprLitFloat::show() { return std::to_string(val); }

void Ast::Nodes::ExprLitFloat::generate(Context& ctx) {}; //TODO

// ExprVar --------------------------------------------------

std::unique_ptr<Ast::Nodes::ExprVar> Ast::Nodes::ExprVar::parse(Lexer::Stream& s) { 
    return std::make_unique<ExprVar>(
        std::make_unique<std::string>(s.pop().content)
    ); 
}

std::string Ast::Nodes::ExprVar::show() { return *name; }

void Ast::Nodes::ExprVar::generate(Context& ctx) {
    uint64_t var_addr = ctx.var(*name) * 8;
    ctx.emit("mov rax, [vars + " + std::to_string(var_addr) + "]");
};

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

    return std::make_unique<Let>(std::make_unique<std::string>(target), std::move(expr));
}

std::string Ast::Nodes::Let::show() {
    return "let " + *target + " = " + expr->show() + ";";
}

void Ast::Nodes::Let::generate(Context& ctx) {
    uint64_t var_addr = ctx.var(*target) * 8;
    
    expr->generate(ctx);
    ctx.emit("mov [vars + " + std::to_string(var_addr) + "], rax");
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

std::string Ast::Program::show() {
    std::string out;

    for (auto& node : content)
        out += (node->show() + "\n");

    return out;
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& expr : content)
        expr->generate(ctx);
}