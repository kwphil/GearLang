#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <format>

#include "../ast.hpp"
#include "../lex.hpp"

std::unique_ptr<Ast::Nodes::Expr> Ast::Nodes::Expr::parse(Lexer::Stream& s) {
    return parseExpr(s);
}

std::unique_ptr<Ast::Nodes::ExprLitInt> Ast::Nodes::ExprLitInt::parse(Lexer::Stream& s) 
{ return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop()->content), s.peek()->line); }

std::unique_ptr<Ast::Nodes::ExprLitFloat> Ast::Nodes::ExprLitFloat::parse(Lexer::Stream& s) { 
    double val = std::stod(s.pop()->content);

    return std::make_unique<Ast::Nodes::ExprLitFloat>(val, s.peek()->line); 
}

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