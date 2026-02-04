#include <memory>
#include <tuple>

#include "../ast.hpp"
#include "../var.hpp"
#include "../lex.hpp"
#include "../error.hpp"

// Helper function for parsing function headers
// Returns the type (returns Ast::Type::NonPrimitive, ... if non-primitive)
// Returns the name of the function
std::tuple<Ast::Type, std::string> 
parse_function_header(Lexer::Stream& s, int line_number);

// Helper function
// Parses the arguments for a function
std::vector<Ast::Variable> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names
);

std::unique_ptr<Ast::Nodes::Function>
Ast::Nodes::Function::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;

    s.expect("fn", line_number);

    Ast::Type ty;
    std::string name;
    std::vector<Ast::Variable> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, true);

    auto block = NodeBase::parse(s);
    return std::make_unique<Function>(name, ty, args, std::move(block), line_number);
}

std::unique_ptr<Ast::Nodes::ExternFn> Ast::Nodes::ExternFn::parse(Lexer::Stream& s) {
    int line_number = s.peek()->line;
    
    s.expect("extern", line_number);
    s.expect("fn", line_number);

    Ast::Type ty;
    std::string name;
    std::vector<Ast::Variable> args;

    auto header = parse_function_header(s, line_number);

    ty = std::get<0>(header);
    name = std::get<1>(header);

    args = parse_function_args(s, line_number, false);

    return std::make_unique<ExternFn>(name, ty, args, line_number);
}

// PRIVATE FUNCTIONS


std::tuple<Ast::Type, std::string> 
parse_function_header(Lexer::Stream& s, int line_number) {
    // Skipping over the current token, is there a parameter list or a block? 
    // If there is not, then there is a type included
    Ast::Type ty;
    
    auto t = s.peek();
    if(s.next()->content != ":" && s.next()->type != Lexer::Type::BraceOpen) {
        ty = Ast::Type(s);
    }

    std::string name = s.pop()->content;

    return { ty, name };
}

std::vector<Ast::Variable> parse_function_args(
    Lexer::Stream& s, 
    int line_number,
    bool requires_names
) {
    // No arguments
    if (s.peek()->content != ":") {
        return { };
    }

    // Consume ':'
    s.pop();

    std::vector<Ast::Variable> args;

    while (true) {
        if (!s.has()) {
            Error::throw_error(
                line_number,
                "",
                "Unexpected EOF parsing args for function",
                Error::ErrorCodes::UNEXPECTED_EOF
            );
        }

        std::string arg_name = s.pop()->content;

        Ast::Type arg_type(s);  

        args.push_back({ arg_name, arg_type });

        if (s.peek()->content == "{" || s.peek()->content == ";") break;
        s.expect(",", line_number);
    }

    return args;
}