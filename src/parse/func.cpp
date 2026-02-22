#include <memory>
#include <tuple>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>

#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

// Helper function for parsing function headers
// Returns the type (returns Ast::Type::NonPrimitive, ... if non-primitive)
// Returns the name of the function
std::tuple<Sem::Type, std::string> 
parse_function_header(Lexer::Stream& s, int line_number);

// Helper function
// Parses the arguments for a function
std::vector<Sem::Variable> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names,
    bool& is_variadic
);

std::unique_ptr<Ast::Nodes::Function>
Ast::Nodes::Function::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    bool is_variadic;
    bool no_mangle;

    s.expect("fn", line_number);

    Sem::Type ty;
    std::string name;
    std::vector<Sem::Variable> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, true, is_variadic);

    auto block = NodeBase::parse(s);
    return std::make_unique<Function>(name, ty, args, std::move(block), is_variadic, line_number);
}

#include <iostream>

std::unique_ptr<Ast::Nodes::ExternFn> Ast::Nodes::ExternFn::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    bool is_variadic;
    bool no_mangle = false;
    
    s.expect("extern", line_number);
    
    if(s.peek()->type == Lexer::Type::StringLiteral) {
        // Using strcmp because my strings have NULLs
        // that screw everything up with std::string::operator==
        if(strcmp(s.peek()->content.c_str(), "C") == 0) {
            no_mangle = true;
            s.pop();
            goto after_extern;
        } 

        throw std::runtime_error(s.peek()->content);
    }
after_extern:

    s.expect("fn", line_number);

    Sem::Type ty;
    std::string name;
    std::vector<Sem::Variable> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, false, is_variadic);

    return std::make_unique<ExternFn>(name, ty, args, is_variadic, no_mangle, line_number);
}

// PRIVATE FUNCTIONS


std::tuple<Sem::Type, std::string> 
parse_function_header(Lexer::Stream& s, int line_number) {
    // Skipping over the current token, is there a parameter list or a block? 
    // If there is not, then there is a type included
    Sem::Type ty;
    
    auto t = s.peek();
    if(s.next()->content != ":" && s.next()->type != Lexer::Type::BraceOpen) {
        ty = Sem::Type(s);
    }

    std::string name = s.pop()->content;

    return { ty, name };
}

std::vector<Sem::Variable> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names, // TODO: Implement this
    bool& is_variadic
) {
    is_variadic = false;

    // No arguments
    if (s.peek()->content != ":") {
        return { };
    }

    // Consume ':'
    s.pop();

    std::vector<Sem::Variable> args;

    while (true) {
        if (!s.has()) {
            Error::throw_error(
                line_number,
                "",
                "Unexpected EOF parsing args for function",
                Error::ErrorCodes::UNEXPECTED_EOF
            );
        }

        if (s.peek()->type == Lexer::Type::Ellipsis) {
            s.pop(); // Get rid of the ellipsis
            is_variadic = true;
            break; // Will be the last parameter
        }
        
        std::string arg_name = s.pop()->content;

        Sem::Type arg_type(s);  

        args.push_back({ arg_name, arg_type });
        
        if (s.peek()->content == "{" || s.peek()->content == ";") break;
        s.expect(",", line_number);
    }

    return args;
}
