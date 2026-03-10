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
#include "expr.hpp"

#include <gearlang/lex.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/type.hpp>

using std::unique_ptr;
using std::weak_ptr;

namespace Ast::Nodes {
    /// @brief Expression node for variable references
    class ExprVar : public Expr {
    protected:
        /// @brief the Let statement this references
        NodeBase* let;

    public:
        /// @brief The variable name
        const std::string name;

        ExprVar(const std::string& name, Span span, unique_ptr<Sem::Type> ty = nullptr)
        : Expr(span, std::move(ty)), name(name) {};

        /// @brief Returns the alloca or global this variable references WITHOUT loading it
        /// @returns an AllocaInst or GlobalVariable or whatever else
        virtual llvm::Value* access_alloca(Context& ctx);

        static std::unique_ptr<ExprVar> parse(const Lexer::Token& name, Lexer::Stream& s);
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

        /// @brief s.x, s.y, etc.
    class ExprStructParam : public ExprVar {
    private:
        /// @brief the struct name
        string struct_name;
        /// @brief the index of the param
        int index;
    
    public:
        ExprStructParam(string struct_name, string param_name, Span span)
        : ExprVar(param_name, span), struct_name(struct_name) { }

        virtual llvm::Value* access_alloca(Context& ctx) override;
        static std::unique_ptr<ExprStructParam> parse(const Lexer::Token& name, Lexer::Stream& s);
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for variable assignments
    class ExprAssign : public Expr {
    private:
        /// @brief The variable to assign
        unique_ptr<ExprVar> var;
        /// @brief The expression to assign to the variable
        pExpr expr;
        /// @brief The Let statement this references
        NodeBase* let;

    public:
        ExprAssign(unique_ptr<ExprVar> var, pExpr expr, Span span)
        : Expr(span), var(std::move(var)), expr(std::move(expr)) { }

        static std::unique_ptr<ExprAssign> parse(unique_ptr<ExprVar>, Lexer::Stream& s);
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

        /// @brief Expression node for referencing variables
    class ExprAddress : public Expr {
    private:
        /// @brief The variable to assign
        unique_ptr<ExprVar> var;

    public:
        /// @brief the Let statement this references
        NodeBase* let;

        ExprAddress(unique_ptr<ExprVar> var, Span span)
        : Expr(span), var(std::move(var)) { }

        static std::unique_ptr<ExprAddress> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for the variable to dereference
    class ExprDeref : public Expr {
    private:
        /// @brief The variable to assign
        unique_ptr<ExprVar> var;

    public:
        NodeBase* let;

        ExprDeref(unique_ptr<ExprVar> var, Span span)
        : Expr(span), var(std::move(var)) { }

        static std::unique_ptr<ExprDeref> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

        /// @brief Function arguments
    class Argument : public ExprVar {
    public:
        /// @brief the name of the function
        string name;
        /// @brief the argument converted to llvm
        llvm::Value* var;

        Argument(string name, Sem::Type ty, Span span)
        : ExprVar(name, span), name(name) { set_type(ty); }

        static unique_ptr<Argument> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        unique_ptr<Value> generate(Context& ctx) override { return nullptr; } 
        virtual std::string to_string() override;
    };
}
