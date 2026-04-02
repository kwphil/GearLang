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

#include <gearlang/sem/val.hpp>
#include <gearlang/sem/type.hpp>
#include "base.hpp"
#include "stmt.hpp"
#include "vars.hpp"

#include <string>
#include <deque>
#include <memory>

using std::string;
using std::deque;
using std::unique_ptr;

namespace Ast::Nodes {
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
        /// @brief Build with public linkage
        bool is_public;

    public:
        Function(
            string& name, 
            Sem::Type ty, 
            deque<unique_ptr<Argument>>&& args, 
            unique_ptr<NodeBase> block, 
            bool is_variadic,
            Span span,
            bool is_public
        ) : 
            Stmt(span), name(name), ty(ty), args(std::move(args)),
            block(std::move(block)), is_variadic(is_variadic), is_public(is_public) { } 

        static unique_ptr<Function> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
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
            string callee, 
            Sem::Type ty,
            deque<unique_ptr<Argument>> args, 
            bool is_variadic,
            bool no_mangle,
            Span span
        ) : Stmt(span), callee(callee), ty(ty), args(std::move(args)), 
            is_variadic(is_variadic), no_mangle(no_mangle) { }

        static unique_ptr<ExternFn> parse(Lexer::Stream& s);

        virtual void analyze(Sem::Analyzer& analyzer) override;
        llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };
}
