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

#include <format>

#include <gearlang/sem/analyze.hpp>
#include <gearlang/sem/val.hpp>

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/ast/vars.hpp>

#include <gearlang/error.hpp>

using namespace Ast::Nodes;
using namespace Sem;

bool Let::analyze(Analyzer& analyzer) {
    unique_ptr<ExprValue> rvalue;

    if(expr) {
        unique_ptr<ExprValue> rvalue = expr->analyze(analyzer);
        assert(expr->get_type() != std::nullopt);
        Type rvalue_ty = expr->get_type().value();
        
        if(!ty) {
            ty = std::make_unique<Type>(rvalue_ty);

            analyzer.trace(
                { { "kind", "find type" }, { "var", target }, { "type", ty->dump() } },
                Error::ErrorCodes::OK, span_meta
            );
        }
        
        if(!analyzer.type_is_compatible(*ty, rvalue_ty)) {
            Error::throw_error(
                span_meta,
                std::format(
                    "Type mismatch: {} is not compatible with type {}",
                    ty->dump(), rvalue_ty.dump()
                ).c_str(),
                Error::ErrorCodes::BAD_TYPE
            );
            return false;
        }
    }

    if(is_public && !analyzer.is_global_scope()) {
        Error::throw_error(
            span_meta,
            "`export` can not be used on a non-global variable!",
            Error::ErrorCodes::QUALIFIER_NOT_ALLOWED
        );
        return false;
    }

    is_global = is_public 
        ? is_public
<<<<<<< HEAD
        : static_cast<uint8_t>(analyzer.is_global_scope()*2); 
=======
        : static_cast<uint8_t>(analyzer.is_global_scope()); 
>>>>>>> master

    Variable var = {
        .name=target,
        .type=*ty,
        .is_global=is_global,
        .let_stmt=this
    };

    analyzer.trace(
        { 
            { "kind", "declare" }, 
            { "declare", "var" }, 
            { "name", target }, 
<<<<<<< HEAD
            { "global", std::to_string((int)is_global) },
=======
            { "global", std::to_string(is_global) },
>>>>>>> master
        },
        Error::ErrorCodes::OK, span_meta
    );
    analyzer.add_variable(target, var);
    return false;
}

unique_ptr<ExprValue> ExprVar::analyze(Analyzer& analyzer) {
<<<<<<< HEAD
=======
    analyzer.throw_if_in_global(span_meta);
>>>>>>> master
    optional<Variable> var_wrap = analyzer.decl_lookup(name);

    analyzer.trace(
        { { "kind", "search" }, { "search", name }, { "for", "var" }, { "status", "start" } },
        Error::ErrorCodes::OK, span_meta
    );

    if(!var_wrap.has_value()) {
        Error::throw_error_and_recover(
            span_meta,
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
        return nullptr;
    }

    analyzer.trace(
        { { "kind", "search" }, { "search", name }, { "status", "pass" } },
        Error::ErrorCodes::OK, span_meta
    );

    Variable var = var_wrap.value();

    let = var.let_stmt;
    ty = std::make_unique<Type>(var.type);

    return std::make_unique<ExprValue>(false, var.type);
}

unique_ptr<ExprValue> ExprAssign::analyze(Analyzer& analyzer) {
<<<<<<< HEAD
=======
    analyzer.throw_if_in_global(span_meta);
>>>>>>> master
    var->analyze(analyzer);
    assert(var->get_type() != std::nullopt);
    ty = std::make_unique<Type>(var->get_type().value());

    expr->set_type(*ty); // implicit casting
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

bool Struct::analyze(Sem::Analyzer& analyzer) {
    Variable v = { .name=name, .type=ty, .is_global=true, .let_stmt=this };
    analyzer.add_variable(name, v);
    return false;
}

unique_ptr<ExprValue> ExprStructParam::analyze(Sem::Analyzer& analyzer) {
<<<<<<< HEAD
=======
    analyzer.throw_if_in_global(span_meta);
>>>>>>> master
    optional<Variable> v = analyzer.decl_lookup(struct_name);

    analyzer.trace(
        { { "kind", "search" }, { "search", name }, { "for", "var" }, { "status", "start" } },
        Error::ErrorCodes::OK, span_meta
    );

    if(!v) {
        Error::throw_error_and_recover(
            span_meta,
            "Tried to access a struct that doesn't exist!",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
        return nullptr;
    }

    analyzer.trace(
        { { "kind", "search" }, { "search", name }, { "status", "pass" } },
        Error::ErrorCodes::OK, span_meta
    );

    let = v->let_stmt;
    index = v->type.struct_parameter_index(name);

    if(index < 0) {
        Error::throw_error_and_recover(
            span_meta,
            std::format(
                "Struct: {} has no member: {}",
                struct_name, name
            ).c_str(),
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
        return nullptr;
    }

    analyzer.trace(
        { 
            { "kind", "struct index" }, 
            { "struct", v->type.dump() }, 
            { "param", name }, 
            { "index", std::to_string(index) } 
        },
        Error::ErrorCodes::OK, span_meta
    );

    ty = std::make_unique<Type>(v->type);

    return std::make_unique<ExprValue>(false, *ty);
}
