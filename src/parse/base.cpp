#include <string>
#include <vector>
#include <memory>
#include <format>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using std::unique_ptr;
using std::string;

unique_ptr<Let> Let::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    
    s.expect("let", line_number);
    string target = s.pop()->content;
    s.expect("=", line_number);
    unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Let>(target, std::move(expr), line_number);
}

string Let::to_string() {
    return std::format(
        "{{ let target={}, expr={} }}",
        target, expr.value()->to_string()
    );
}

unique_ptr<ExprBlock> ExprBlock::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    std::vector<unique_ptr<NodeBase>> nodes;

    s.expect("{", line_number);
    unique_ptr<Lexer::Token> t = s.peek();
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

    return std::make_unique<ExprBlock>(std::move(nodes), line_number);
}

string ExprBlock::to_string() {
    string out = "{ ExprBlock ";

    for(auto& n : nodes) {
        out += " { ";
        out += n->to_string();
        out += " } ";
    }

    out += " }";

    return out;
}

unique_ptr<NodeBase> NodeBase::parse(Lexer::Stream& s) {
    unique_ptr<NodeBase> out;
    unique_ptr<Lexer::Token> curr = s.peek();
    int line_number = curr->line;

    if(!s.has()) {
        Error::throw_error(
            line_number,
            "",
            "Unexpected EOF",
            Error::ErrorCodes::UNEXPECTED_EOF
        );
    }
    
    // These do not require semicolons, so early return
    if      (curr->content == "fn")     return Function::parse(s);
    else if (curr->content == "{")      return ExprBlock::parse(s);
    else if (curr->content == "if") {
        auto if_expr = If::parse(s);
        
        if(s.peek()->content == "else") {
            return Else::parse(std::move(if_expr), s);
        } 

        return if_expr;
    }
    // These do
    else if (curr->content == "let")    out = Let::parse(s);
    else if (curr->content == "return") out = Return::parse(s);
    else if (curr->content == "extern") out = ExternFn::parse(s);
    else                                out = Expr::parse(s);
    s.expect(";", s.peek()->line);

    return out;
}

unique_ptr<Return> Return::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    
    s.expect("return", line_number);
    unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Return>(std::move(expr), line_number);
}

string Return::to_string() { return std::format("{{ Return expr={} }}", expr->to_string()); }

unique_ptr<If> If::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line; // Get it here on the line of the if statement itself
    s.expect("if", line_number);
    pExpr cond = Expr::parse(s);
    unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<If>(std::move(expr), std::move(cond), line_number);
}

string If::to_string() {
    return std::format("{{ if cond={} expr={} }}", cond->to_string(), expr->to_string());
}

unique_ptr<Else> Else::parse(
    unique_ptr<If> if_expr,
    Lexer::Stream& s
) {
    int line_number = s.peek()->line;
    s.expect("else", line_number);
    unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<Else>(std::move(expr), std::move(*if_expr));
}

string Else::to_string() {
    return std::format("{{ else cond={} exprtrue={} exprfalse={} }}",
        cond->to_string(), expr->to_string(), else_expr->to_string()
    );
}

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has()) {
        that.content.push_back(Nodes::NodeBase::parse(s));
    }
    
    return that; 
}

std::string Ast::Program::to_string() {
    std::string out;
    
    for(auto& n : content) {
        out += n->to_string();
        out += '\n';
    }   

    return out;
}