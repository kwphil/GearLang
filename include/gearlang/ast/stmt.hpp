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
#include <optional>

#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>

#include "base.hpp"
#include "expr.hpp"

using std::optional;
using std::pair;
using Sem::Type;

namespace Sem {
    class Analyzer;
    struct ExprValue;
}

namespace Ast::Nodes {
    class Expr;
    using pExpr = std::unique_ptr<Expr>;

    /// @brief Base class for statements (i.e. functions, ifs and others)
    class Stmt : public NodeBase {
    public:
        Stmt(Span span) : NodeBase(span) { }

        /// @brief For the semantic analyzer
        /// @param analyzer The analyzer object
        virtual void analyze(Sem::Analyzer& analyzer) = 0;

        /// @brief generate function that doesn't return anything
        /// @param ctx the context
        virtual void generate(Context& ctx) = 0;
    };

    /// @brief Node for defining structs
    class Struct : public Stmt {
    private:
        using Arg = pair<string, Type>;

        /// @brief The name of the struct
        string name;
        /// @brief the arguments of the struct
        vector<Arg> args;

    public:
        Struct(string name, Span span, vector<std::pair<string, Sem::Type>> args)
        : Stmt(span), name(name), args(std::move(args)) { }

        static unique_ptr<Struct> parse(Lexer::Stream& s);
        virtual void analyze(Sem::Analyzer& analyzer) override;
        virtual void generate(Context& ctx) override;
    };

    /// @brief Node for if statements
    class If : public Stmt {
    protected:
        /// @brief The condition expression
        pExpr cond;
        /// @brief The expression to execute if the condition is true
        unique_ptr<NodeBase> expr;

    public:
        If(unique_ptr<NodeBase> expr, pExpr cond, Span span)
        : Stmt(span), cond(std::move(cond)), expr(std::move(expr)) { }

        If(If&&) = default;
        If& operator=(If&&) = default;
        If(const If&) = delete;

        static unique_ptr<If> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx) override;

        virtual std::string to_string() override;
    };

    /// @brief If/Else
    class Else : public If {
    private:
        /// @brief The condition expression for false if
        std::unique_ptr<NodeBase> else_expr;

    public:
        Else(
            std::unique_ptr<NodeBase> expr,
            If&& if_expr
        ) : If(std::move(if_expr)), else_expr(std::move(expr)) { }

        static std::unique_ptr<Else> parse(
            std::unique_ptr<If>,
            Lexer::Stream& s
        );

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx);

        virtual std::string to_string() override;
    };

    /// @brief Node for variable declarations
    class Let : public Stmt {
    private:
        /// @brief The target variable name
        std::string target;
        /// @brief The expression for the variable's initial value
        pExpr expr;
        /// @brief The type of the variable
        unique_ptr<Sem::Type> ty;
    public:
        /// @brief The LLVM variable
        llvm::Value* var;

        /// @brief If the variable is to be generated as a global
        bool is_global = false;

        Let(std::string& target, pExpr expr, unique_ptr<Sem::Type> ty, Span span)
        : Stmt(span), target(target), expr(std::move(expr)), ty(std::move(ty)) {}

        static std::unique_ptr<Let> parse(Lexer::Stream& s);

        std::string get_name() { return target; }
        Sem::Type get_type() { return *ty; }
        void generate(Context& ctx) override;
        void analyze(Sem::Analyzer& analyzer) override; 
        virtual std::string to_string() override;
    };

    /// @brief Node for return statements
    class Return : public Stmt {
    private:
        /// @brief The expression to return
        pExpr expr;
    
    public:
        Return(std::unique_ptr<Expr> expr, Span span)
        : Stmt(span), expr(std::move(expr)) {}

        static std::unique_ptr<Return> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        void generate(Context& ctx) override;
        virtual std::string to_string() override;
    };
}
