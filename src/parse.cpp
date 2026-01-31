#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <format>

#include "ast.hpp"
#include "lex.hpp"

std::unique_ptr<Ast::Nodes::Expr> Ast::Nodes::Expr::parse(Lexer::Stream& s) {
    return parseExpr(s);
}

std::unique_ptr<Ast::Nodes::ExprLitInt> Ast::Nodes::ExprLitInt::parse(Lexer::Stream& s) 
{ return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop()->content), s.peek()->line); }

std::unique_ptr<Ast::Nodes::ExprLitFloat> Ast::Nodes::ExprLitFloat::parse(Lexer::Stream& s) { 
    double val = std::stod(s.pop()->content);

    return std::make_unique<Ast::Nodes::ExprLitFloat>(val, s.peek()->line); 
}

#include <iostream>

std::unique_ptr<Ast::Nodes::ExprLitString> Ast::Nodes::ExprLitString::parse(Lexer::Stream& s) {
    std::unique_ptr<Lexer::Token> t = s.pop();

    // the string contains the sorrounding quotes, so let's remove them
    std::string string = t->content.substr(1, t->content.size() - 2); // - 2 because first and last
    
    return std::make_unique<ExprLitString>(string, t->line);
}

std::unique_ptr<Ast::Nodes::ExprVar> Ast::Nodes::ExprVar::parse(const Lexer::Token& token) { 
    return std::make_unique<ExprVar>(token.content, token.line);
}

std::unique_ptr<Ast::Nodes::ExprAssign>
Ast::Nodes::ExprAssign::parse(const Lexer::Token& token, Lexer::Stream& s) {
    s.expect("=");
    pExpr expr = Expr::parse(s);

    return std::make_unique<ExprAssign>(token.content, std::move(expr), token.line);
}

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

    return std::make_unique<ExprOp>(type, std::move(left), std::move(right), s.peek()->line);
}

Ast::Nodes::pExpr Ast::Nodes::Expr::parseTerm(Lexer::Stream& s, llvm::Type* cast) {
    int line_number = s.peek()->line;

    if (s.peek()->content == "(") {
        s.expect("(");
        auto expr = parseExpr(s);
        s.expect(")");
        return expr;
    }

    Lexer::Token lit = *(s.peek());

    switch (lit.type) {
        case Lexer::Type::FloatLiteral:   return ExprLitFloat::parse(s);    break;
        case Lexer::Type::IntegerLiteral: return ExprLitInt::parse(s);      break;
        case Lexer::Type::StringLiteral:  return ExprLitString::parse(s);   break;
        default: break;
    }

    s.pop();

    if(lit.type == Lexer::Type::Identifier) {
        if(lit.content == "if") {
            s.back();
            return If::parse(s);
        }
        
        if(s.peek()->content == "=") {
            return ExprAssign::parse(lit, s);
        }

        return ExprVar::parse(lit);
    }

    std::string error_msg = std::format("Unexpect token: {} (type={})", lit.content, (int)lit.type);
    throw std::runtime_error(error_msg);
}

std::unique_ptr<Ast::Nodes::Let> Ast::Nodes::Let::parse(Lexer::Stream& s) {
    s.expect("let");
    std::string target = s.pop()->content;
    s.expect("=");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Let>(target, std::move(expr), s.peek()->line);
}

std::unique_ptr<Ast::Nodes::ExprBlock> Ast::Nodes::ExprBlock::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
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

    return std::make_unique<Ast::Nodes::ExprBlock>(std::move(nodes), line_number);
}

std::unique_ptr<Ast::Nodes::NodeBase> Ast::Nodes::NodeBase::parse(Lexer::Stream& s) {
    std::unique_ptr<NodeBase> out;
    std::unique_ptr<Lexer::Token> curr = s.peek();

    if(!s.has()) {
        throw std::runtime_error("Ast::Nodes::NodeBase::parse, unexpected end of stream");
    }

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
    else                                out = Expr::parse(s);
    s.expect(";", out);

    return out;
}

std::unique_ptr<Ast::Nodes::Return> Ast::Nodes::Return::parse(Lexer::Stream& s) {
    s.expect("return");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Return>(Return(std::move(expr), s.peek()->line));
}

std::unique_ptr<Ast::Nodes::If> Ast::Nodes::If::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line; // Get it here on the line of the if statement itself
    s.expect("if");
    pExpr cond = Expr::parse(s);
    std::unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<If>(If(std::move(expr), std::move(cond), line_number));
}

std::unique_ptr<Ast::Nodes::Function> Ast::Nodes::Function::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line; // Get it here on the line of the fn statement itself
    s.expect("fn");
    std::string name = s.pop()->content;
    // TODO: Parse variable list
    std::unique_ptr<NodeBase> block = NodeBase::parse(s); 

    return std::make_unique<Function>(Function(name, std::move(block), line_number));
}

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has()) {
        that.content.push_back(Nodes::NodeBase::parse(s));
    }
    
    return that; 
}
