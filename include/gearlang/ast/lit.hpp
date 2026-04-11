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

namespace Ast::Nodes {
    /// @brief Template base class for literal expressions
    class Literal : public Expr {
    protected:
        llvm::Type* cast_type;
        
    public:
        Literal(Span span, llvm::Type* cast) 
        : Expr(span), cast_type(cast) { }
        
        virtual ~Literal() = default;
    
        static std::unique_ptr<Literal> parse(Lexer::Stream& s, llvm::Type* cast = nullptr);
    };

    /// @brief Expression node for integer literals
    class ExprLitInt : public Literal {
    public:
        uint64_t value;

        ExprLitInt(uint64_t x, Span span) : Literal(span, nullptr), value(x) {}
        static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for floating-point literals
    class ExprLitFloat : public Literal {
    public:
        double value;

        ExprLitFloat(double x, Span span) : Literal(span, nullptr), value(x) { }
        static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s);
        
        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for C-strings 
    class ExprLitString : public Literal {
    private:
        std::string string;

    public:
        ExprLitString(std::string& s, Span span) : Literal(span, nullptr), string(s) { }
        static std::unique_ptr<ExprLitString> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };

    /// @brief Expression node for char
    class ExprLitChar : public Literal {
    private:
        char c;
    
    public:
        ExprLitChar(char c, Span span) : Literal(span, nullptr), c(c) { }
        static std::unique_ptr<ExprLitChar> parse(Lexer::Stream& s);

        virtual unique_ptr<Sem::ExprValue> analyze(Sem::Analyzer& analyzer) override;
        virtual llvm::Value* generate(Context& ctx) override;
        virtual std::string to_string() override;
    };
};
