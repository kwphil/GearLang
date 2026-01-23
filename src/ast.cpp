#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <format>

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

// ExprLitInt ---------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitInt> Ast::Nodes::ExprLitInt::parse(Lexer::Stream& s) 
{ return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop()->content)); }

std::string Ast::Nodes::ExprLitInt::show() 
{ return std::to_string(this->val); } 

// ExprLitFloat ------------------------------------------

std::unique_ptr<Ast::Nodes::ExprLitFloat> Ast::Nodes::ExprLitFloat::parse(Lexer::Stream& s) 
{ return std::make_unique<Ast::Nodes::ExprLitFloat>(std::stod(s.pop()->content)); }

std::string Ast::Nodes::ExprLitFloat::show() { return std::to_string(val); }

// ExprVar --------------------------------------------------

std::unique_ptr<Ast::Nodes::ExprVar> Ast::Nodes::ExprVar::parse(std::string& name) { 
    return std::make_unique<ExprVar>(name);
}

std::string Ast::Nodes::ExprVar::show() { return name; }

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

// Expr ------------------------------------------------

Ast::Nodes::pExpr Ast::Nodes::Expr::parseExpr(Lexer::Stream& s) {
    pExpr left = parseTerm(s);
    
    //no operator
    if (s.peek()->type != Lexer::Type::Operator)
        return left;

    
    ExprOp::Type type;
    switch(s.pop()->content[0])
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
    if (s.peek()->content == "(") {
        s.expect("(");
        auto expr = parseExpr(s);
        s.expect(")");
        return expr;
    }

    Lexer::Token lit = *(s.peek());

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

        if(s.peek()->content == "=") {
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
    std::string target = s.pop()->content;
    s.expect("=");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Let>(target, std::move(expr));
}

std::string Ast::Nodes::Let::show() {
    return "let " + target + " = " + expr->show() + ";";
}

// ExprBlock ------------------------------------------------------

std::unique_ptr<Ast::Nodes::ExprBlock> Ast::Nodes::ExprBlock::parse(Lexer::Stream& s) {
    std::vector<std::unique_ptr<NodeBase>> nodes;

    s.expect("{");
    std::unique_ptr<Lexer::Token> t = s.peek();
    int brace_count = 1;
    while(s.has()) {
        // For nested braces
        if(t->type == Lexer::Type::BraceOpen) brace_count++;
        if(t->type == Lexer::Type::BraceClose) brace_count--;

        if(brace_count == 0) {
            break;
        }

        nodes.push_back(NodeBase::parse(s));

        t = s.peek();
    }

    s.pop(); // To remove the last }

    return std::make_unique<Ast::Nodes::ExprBlock>(std::move(nodes));
}

std::string Ast::Nodes::ExprBlock::show() {
    std::string node_show;

    for(const auto& expr : nodes)
        node_show += expr->show();

    return "{\n" + node_show + "\n}\n";
}

std::unique_ptr<Ast::Nodes::NodeBase> Ast::Nodes::NodeBase::parse(Lexer::Stream& s) {
    std::unique_ptr<NodeBase> out;
    std::unique_ptr<Lexer::Token> curr = s.peek();

    if(curr == nullptr) {
        throw std::runtime_error("Ast::Nodes::NodeBase::parse, unexpected nullptr at s.peek()");
    }

    // These do not require semicolons, so early return
    if      (curr->content == "fn")     return Function::parse(s);
    else if (curr->content == "{")      return ExprBlock::parse(s);
    else if (curr->content == "if")     return If::parse(s);
    // These do
    else if (curr->content == "let")    out = Let::parse(s);
    else if (curr->content == "return") out = Return::parse(s);
    else if (curr->content == "if")     out = If::parse(s); 
    else                                out = Expr::parse(s);
    s.expect(";", out);

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

// If --------------------------------------------

std::unique_ptr<Ast::Nodes::If> Ast::Nodes::If::parse(Lexer::Stream& s) {
    s.expect("if");
    pExpr cond = Expr::parse(s);
    std::unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<If>(If(std::move(expr), std::move(cond)));
}

std::string Ast::Nodes::If::show() {
    return "if" + cond->show() + " --- " + expr->show();
}

// Function ----------------------------------------------

std::unique_ptr<Ast::Nodes::Function> Ast::Nodes::Function::parse(Lexer::Stream& s) {
    s.expect("fn");
    std::string name = s.pop()->content;
    // TODO: Parse variable list
    std::unique_ptr<NodeBase> block = NodeBase::parse(s); 

    return std::make_unique<Function>(Function(name, std::move(block)));
}

std::string Ast::Nodes::Function::show() {
    return "fn " + name + " { " + block->show() + " }";
}

// Program -------------------------------------------------

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
