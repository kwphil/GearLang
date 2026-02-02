#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <format>

#include "../ast.hpp"
#include "../lex.hpp"
#include "../var.hpp"

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
    else if (curr->content == "extern") out = ExternFn::parse(s);
    else                                out = Expr::parse(s);
    s.expect(";", out);

    return out;
}

std::unique_ptr<Ast::Nodes::Return> Ast::Nodes::Return::parse(Lexer::Stream& s) {
    s.expect("return");
    std::unique_ptr<Expr> expr = Expr::parse(s);

    return std::make_unique<Return>(std::move(expr), s.peek()->line);
}

std::unique_ptr<Ast::Nodes::If> Ast::Nodes::If::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line; // Get it here on the line of the if statement itself
    s.expect("if");
    pExpr cond = Expr::parse(s);
    std::unique_ptr<NodeBase> expr = NodeBase::parse(s);

    return std::make_unique<If>(std::move(expr), std::move(cond), line_number);
}

        #include <iostream>
std::unique_ptr<Ast::Nodes::Function>
Ast::Nodes::Function::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;

    s.expect("fn");

    Ast::Type ty = Ast::Type::Void;
    Ast::NonPrimitive npty;

    if (s.peek()->type == Lexer::Type::Identifier) {
        auto first = s.pop();

        if (s.peek()->type == Lexer::Type::Identifier) {
            s.back();
            ty = parse_type(s);
            
            if(ty == Ast::Type::NonPrimitive) {
                npty = parse_nonprim(s);
            }

            if (ty == Ast::Type::Invalid) {
                throw std::runtime_error("Unknown type: " + first->content);
            }
        } else {
            // not a return type, rewind
            s.back();
        }
    }

    std::string name = s.pop()->content;
    std::vector<Ast::Variable> args;

    // No arguments
    if (s.peek()->content != ":") {
        goto fn_parse_end;
    }

    // Consume ':'
    s.pop();

    while (true) {
        if (!s.has()) {
            throw std::runtime_error(
                "Program ended early parsing args for function: " + name
            );
        }

        std::string arg_name = s.pop()->content;

        Ast::Type arg_type = parse_type(s);
        Ast::NonPrimitive arg_npty;        


        if (arg_type == Ast::Type::Invalid) {
            s.back(); // Going back to what was parsed
            throw std::runtime_error("Unknown type: " + s.pop()->content);
        }
 
        if(arg_type == Ast::Type::NonPrimitive) {
            args.push_back({ arg_name, arg_type, parse_nonprim(s)});
        } else {
            args.push_back({ arg_name, arg_type, Ast::NonPrimitive({0}) });
        }

        if (s.peek()->content == "{") break;
        s.expect(",");
    }

    
fn_parse_end:
    auto block = NodeBase::parse(s);
    return std::make_unique<Function>(name, ty, npty, args, std::move(block), line_number);
}

std::unique_ptr<Ast::Nodes::ExternFn> Ast::Nodes::ExternFn::parse(Lexer::Stream& s) {
    s.expect("extern");
    s.expect("fn");

    int line_number = s.peek()->line;


    Ast::Type ty = Ast::Type::Void;
    Ast::NonPrimitive npty;

    if (s.peek()->type == Lexer::Type::Identifier) {
        auto first = s.pop();

        if (s.peek()->type == Lexer::Type::Identifier) {
            s.back();
            ty = parse_type(s);
            
            if(ty == Ast::Type::NonPrimitive) {
                npty = parse_nonprim(s);
            }

            if (ty == Ast::Type::Invalid) {
                throw std::runtime_error("Unknown type: " + first->content);
            }
        } else {
            // not a return type, rewind
            s.back();
        }
    }

    std::string name = s.pop()->content;
    std::vector<Ast::Variable> args;

    // No arguments
    if (s.peek()->content != ":") {
        goto fn_parse_end;
    }

    // Consume ':'
    s.pop();

    while (true) {
        if (!s.has()) {
            throw std::runtime_error(
                "Program ended early parsing args for function: " + name
            );
        }

        std::string arg_name = s.pop()->content;

        Ast::Type arg_type = parse_type(s);
        Ast::NonPrimitive arg_npty;        


        if (arg_type == Ast::Type::Invalid) {
            s.back(); // Going back to what was parsed
            throw std::runtime_error("Unknown type: " + s.pop()->content);
        }
 
        if(arg_type == Ast::Type::NonPrimitive) {
            args.push_back({ arg_name, arg_type, parse_nonprim(s)});
        } else {
            args.push_back({ arg_name, arg_type, Ast::NonPrimitive({0}) });
        }

        if (s.peek()->content == ";") break;
        s.expect(",");
    }
    
fn_parse_end:
    return std::make_unique<ExternFn>(name, ty, npty, args, line_number);
}

Ast::Program Ast::Program::parse(Lexer::Stream& s) {
    Program that = Program();
    while (s.has()) {
        that.content.push_back(Nodes::NodeBase::parse(s));
    }
    
    return that; 
}