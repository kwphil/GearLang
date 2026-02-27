#include <gearlang/sem/val.hpp>
#include <gearlang/sem/type.hpp>
#include "base.hpp"
#include "stmt.hpp"

#include <string>
#include <deque>
#include <memory>

using std::string;
using std::deque;
using std::unique_ptr;

namespace Ast::Nodes {
    /// @brief Function arguments
    class Argument : public Expr {
    public:
        /// @brief the name of the function
        string name;
        /// @brief the argument converted to llvm
        llvm::Value* var;

        Argument(string name, Sem::Type* ty, int line_number)
        : name(name), Expr(line_number, ty) { }

        static unique_ptr<Argument> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override { return nullptr; } 
    };

    /// @brief Node for function definitions
    class Function : public Stmt {
    private:
        /// @brief The function name
        string name;
        /// @brief The function return type
        Sem::Type ty;
        /// @brief The function arguments
        deque<unique_ptr<Argument>> args;
        /// @brief The function body block
        unique_ptr<NodeBase> block;
        /// @brief If the function is variadic
        bool is_variadic;

    public:
        Function(
            string& name, 
            Sem::Type ty, 
            deque<unique_ptr<Argument>>&& args, 
            unique_ptr<NodeBase> block, 
            bool is_variadic,
            int line_number
        ) : 
            name(name), ty(ty), args(std::move(args)), is_variadic(is_variadic),
            block(std::move(block)), Stmt(line_number) { } 

        static unique_ptr<Function> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };

    class ExternFn : public Stmt {
    private:
        /// @brief the callee name
        string callee;
        /// @brief the function return type
        Sem::Type ty;
        /// @brief args
        deque<unique_ptr<Argument>> args;
        /// @brief is_variadic
        bool is_variadic;
        /// @brief not implemented yet, but forces no name mangling
        bool no_mangle;

    public:
        ExternFn(
            string& callee, 
            Sem::Type ty,
            deque<unique_ptr<Argument>>& args, 
            bool is_variadic,
            bool no_mangle,
            int line_number
        ) : callee(callee), args(std::move(args)), ty(ty), 
            is_variadic(is_variadic), no_mangle(no_mangle),
            Stmt(line_number) { }

        static unique_ptr<ExternFn> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        // This has no use for generating code, so this always returns nullptr
        void generate(Context& ctx) override;
    };
}