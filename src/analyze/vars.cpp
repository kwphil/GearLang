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

#include <optional>
#include <format>

#include <gearlang/sem/analyze.hpp>
#include <gearlang/sem/val.hpp>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>

#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using namespace Sem;

void Let::analyze(Analyzer& analyzer) {
    unique_ptr<ExprValue> rvalue;

    if(expr) {
        unique_ptr<ExprValue> rvalue = expr->analyze(analyzer);
        Type rvalue_ty = expr->get_type().value();
        
        if(!ty) 
            ty = std::make_unique<Type>(rvalue_ty); 

        if(!analyzer.type_is_compatible(*ty, rvalue_ty)) {
            Error::throw_error(
                span_meta,
                std::format(
                    "Type mismatch: {} is not compatible with type {}",
                    ty->dump(), rvalue_ty.dump()
                ).c_str(),
                Error::ErrorCodes::BAD_TYPE
            );
        }
    }

    Variable var = {
        .name=target,
        .type=*ty,
        .is_global=static_cast<uint8_t>(analyzer.is_global_scope()*2),
        .let_stmt=this
    };

    analyzer.add_variable(target, var);
}

unique_ptr<ExprValue> ExprVar::analyze(Analyzer& analyzer) {
    optional<Variable> var_wrap = analyzer.decl_lookup(name);

    if(!var_wrap.has_value()) {
        Error::throw_error(
            span_meta,
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = var_wrap.value();

    let = var.let_stmt;
    ty = std::make_unique<Type>(var.type);

    return std::make_unique<ExprValue>(false, var.type);
}

unique_ptr<ExprValue> ExprAssign::analyze(Analyzer& analyzer) {
    optional<Variable> var_wrap = analyzer.decl_lookup(name);

    if(!var_wrap.has_value()) {
        Error::throw_error(
            span_meta,
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    Variable var = var_wrap.value();

    let = var.let_stmt;
    ty = std::make_unique<Type>(var.type);

    return expr->analyze(analyzer);
}

unique_ptr<ExprValue> Argument::analyze(Analyzer& analyzer) {
    Variable var = {
        .name=name,
        .type=*ty,
        .is_global=false,
        .let_stmt=this
    };

    analyzer.add_variable(name, var);

    return nullptr;
}
