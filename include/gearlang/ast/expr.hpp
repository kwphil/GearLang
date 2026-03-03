/*
   _____                 _                       
  / ____|               | |                      
 | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
 | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
 | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
  \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
                                            __/ |
                                           |___/ 

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <memory>

#include "base.hpp"

#include <gearlang/lex.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/type.hpp>

using std::unique_ptr;

namespace Sem {
    class Analyzer;
    struct ExprValue;
}

namespace Ast::Nodes {
    /// @brief Base class for all nodes that return a value
    class Expr : public NodeBase {
    protected:
        /// @brief an expression will expect to return some kind of value
        unique_ptr<Sem::Type> ty;
    
    public:
        /// @brief Wraps the type as an optional for better debugging. Note that this should be called AFTER analyze()
        /// @returns an optional type
        optional<Sem::Type> get_type() { if(!ty) return std::nullopt; return *ty; }
        /// @brief Sets the type
        /// @param ty the type to replace the current type
        void set_type(Sem::Type ty) { this->ty = std::make_unique<Sem::Type>(ty); }

        Expr(int line_number, Sem::Type* ty = nullptr) : NodeBase(line_number), ty(ty) {}

        static std::unique_ptr<Expr> parse(Lexer::Stream& s);

        /// @brief Parses an expression from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed expression
        static std::unique_ptr<Expr> parseExpr(Lexer::Stream& s);
        /// @brief Parses a term from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed term
        static std::unique_ptr<Expr> parseTerm(Lexer::Stream& s);
        /// @brief Parses an expression. Exprs are expected to return some data for other operations it links to
        /// @param analyzer A link to the analyzer
        /// @returns Some metadata about the expression that was parsed
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) = 0;
        /// @brief Generates the llvm code
        virtual unique_ptr<Value> generate(Context& ctx) = 0;
    };

    /// @brief Smart pointer type for expressions
    using pExpr = std::unique_ptr<Expr>;

    /// @brief Expression node for binary operations
    class ExprOp : public Expr {
    public:
        /// @brief Type of binary operation
        enum Type { Add, Sub, Mul, Div, Gt, Lt, Ge, Le, Eq, Ne } type;
        /// @brief operands
        std::unique_ptr<Expr> left, right;

        ExprOp(Type type, pExpr left, pExpr right, int line_number, Sem::Type* ty = nullptr)
        : Expr(line_number, ty), type(type), left(std::move(left)), right(std::move(right)) {};

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Template base class for literal expressions
    class Literal : public Expr {
    protected:
        llvm::Type* cast_type;
        
    public:
        Literal(int line_number, llvm::Type* cast) 
        : Expr(line_number), cast_type(cast) { }
        
        virtual ~Literal() = default;
    
        static std::unique_ptr<Literal> parse(Lexer::Stream& s, llvm::Type* cast = nullptr);
    };

    /// @brief Expression node for integer literals
    class ExprLitInt : public Literal {
    private:
        uint64_t value;

    public:
        ExprLitInt(uint64_t x, int line_number) : Literal(line_number, nullptr), value(x) {}
        static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for floating-point literals
    class ExprLitFloat : public Literal {
    private:
        double value;

    public:
        ExprLitFloat(double x, int line_number) : Literal(line_number, nullptr), value(x) { }
        static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s);
        
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for C-strings 
    class ExprLitString : public Literal {
    private:
        std::string string;

    public:
        ExprLitString(std::string& s, int line_number) : Literal(line_number, nullptr), string(s) { }
        static std::unique_ptr<ExprLitString> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for variable references
    class ExprVar : public Expr {
    private:
        /// @brief The variable name
        const std::string name;
        /// @brief the Let statement this references
        NodeBase* let;

    public:
        ExprVar(const std::string& name, int line_number)
        : Expr(line_number), name(name) {};

        static std::unique_ptr<ExprVar> parse(const Lexer::Token& name);
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for variable assignments
    class ExprAssign : public Expr {
    private:
        /// @brief The variable name
        const std::string name;
        /// @brief The expression to assign to the variable
        pExpr expr;
        /// @brief The Let statement this references
        NodeBase* let;

    public:
        ExprAssign(const std::string& name, pExpr expr, int line_number)
        : Expr(line_number), name(name), expr(std::move(expr)) { }

        static std::unique_ptr<ExprAssign> parse(const Lexer::Token& name, Lexer::Stream& s);
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    class ExprCall : public Expr {
    private:
        /// @brief the arguments
        std::vector<pExpr> args;
        /// @brief the name of the function to call
        std::string callee;
    
    public:
        ExprCall(
            std::string& callee, 
            std::vector<pExpr>& args, 
            int line_number
        ) 
        : Expr(line_number), args(std::move(args)), callee(callee) { }

        static std::unique_ptr<ExprCall> parse(
            const Lexer::Token& name,
            Lexer::Stream& s
        );

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for referencing variables
    class ExprAddress : public Expr {
    private:
        /// @brief the name of the variable to reference
        std::string name;
    public:
        /// @brief the Let statement this references
        NodeBase* let;

        ExprAddress(std::string& name, int line_number)
        : Expr(line_number), name(name) { }

        static std::unique_ptr<ExprAddress> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for the variable to dereference
    class ExprDeref : public Expr {
    private:
        /// @brief the name of the variable to dereference
        std::string name;

    public:
        NodeBase* let;

        ExprDeref(std::string& name, int line_number)
        : Expr(line_number), name(name) { }

        static std::unique_ptr<ExprDeref> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for blocks of nodes
    class ExprBlock : public Expr {
    private:
        /// @brief The list of nodes in the block
        std::vector<std::unique_ptr<NodeBase>> nodes;
    
    public:
        ExprBlock(std::vector<std::unique_ptr<NodeBase>>&& nodes, int line_number)
        : Expr(line_number), nodes(std::move(nodes)) { }

        static std::unique_ptr<ExprBlock> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        // Will probably return the return variable
        // Will stay void until I get Function to return non-void
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };
}
