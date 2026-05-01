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
#include <string>
#include <format>

#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>
#include <gearlang/ffi/c.hpp>

#include "base.hpp"
#include "expr.hpp"
#include "stmt.hpp"

using std::optional;
using std::pair;
using std::string;
using std::vector;
using Sem::Type;

namespace Sem {
    class Analyzer;
    struct ExprValue;
}

namespace Ast::Nodes {
    /// @brief Expression node for blocks of nodes
    class Block : public Stmt {
    private:
        /// @brief The list of nodes in the block
        std::vector<std::unique_ptr<NodeBase>> nodes;
    
    public:
        Block(std::vector<std::unique_ptr<NodeBase>>&& nodes, Span span)
        : Stmt(span), nodes(std::move(nodes)) { }

        static std::unique_ptr<Block> parse(Lexer::Stream& s);

        virtual bool analyze(Sem::Analyzer& analyzer) override;
        llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    class While : public Stmt {
    private:
        std::unique_ptr<NodeBase> code;
        std::unique_ptr<Expr> cond;

    public:
        While(std::unique_ptr<NodeBase> code, std::unique_ptr<Expr> cond, Span span)
        : Stmt(span), code(std::move(code)), cond(std::move(cond)) { }

        static std::unique_ptr<While> parse(Lexer::Stream& s);

        virtual bool analyze(Sem::Analyzer& analyzer);
        llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override { return ""; }
    };

    class Do : public Stmt {
    private:
        std::unique_ptr<NodeBase> code;
        std::unique_ptr<Expr> cond;

    public:
        Do(std::unique_ptr<NodeBase> code, std::unique_ptr<Expr> cond, Span span)
        : Stmt(span), code(std::move(code)), cond(std::move(cond)) { }

        static std::unique_ptr<Do> parse(Lexer::Stream& s);

        virtual bool analyze(Sem::Analyzer& analyzer);
        llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override { return ""; };
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

        virtual bool analyze(Sem::Analyzer& analyzer) override;
        llvm::Value* generate(Context& ctx) override;

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

        virtual bool analyze(Sem::Analyzer& analyzer) override;
        llvm::Value* generate(Context& ctx);

        virtual std::string to_string() override;
    };
}