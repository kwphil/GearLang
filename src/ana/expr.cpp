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

#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/vars.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/error.hpp>
#include <gearlang/func.hpp>

#include <iostream>
#include <optional>
#include <format>

using namespace Ast::Nodes;
using namespace Sem;
using std::optional;

unique_ptr<ExprValue> ExprCall::analyze(Analyzer& analyzer) {
    optional<Func> ref = analyzer.lookup_func(callee);

    analyzer.throw_if_in_global(span_meta);

    analyzer.trace(
        { { "type", "search" }, { "search", callee }, { "status", "start" }, { "for", "function" } }, 
        Error::ErrorCodes::OK, 
        span_meta
    );

    if(!ref) {
        analyzer.trace(
            { { "type", "search" }, { "search", callee }, { "status", "failed" } }, 
            Error::ErrorCodes::FUNCTION_NOT_DEFINED, 
            span_meta
        );

        Error::throw_error_and_recover(
            span_meta,
            "Function not defined",
            Error::ErrorCodes::FUNCTION_NOT_DEFINED
        );

        return nullptr;
    }

    analyzer.trace(
        { { "type", "search" }, { "search", callee }, { "status", "pass" } },
        Error::ErrorCodes::OK,
        span_meta
    );

    Func handle = ref.value();

    identifier = mangle_identifier(handle);

    for(auto& arg : args) {
        arg->analyze(analyzer);
    } 

    if(handle.args.size() != args.size() && !handle.is_variadic) {
        Error::throw_error_and_recover(
            span_meta, 
            std::format(
                "Expected {} elements, received {}",
                handle.args.size(), args.size()
            ).c_str(),
            Error::ErrorCodes::FUNCTION_INVALID_ARGS
        );
        return nullptr;
    }

    if(handle.is_variadic && handle.args.size() > args.size()) {
        Error::throw_error_and_recover(
            span_meta,
            std::format(
                "Expected at least {} elements, received {}",
                handle.args.size(), args.size()
            ).c_str(),
            Error::ErrorCodes::FUNCTION_INVALID_ARGS
        );
        return nullptr;
    }

    auto it = args.begin();
    for(auto& a : handle.args) {
        auto curr_type = (*it)->get_type();
        assert(curr_type != std::nullopt);
        if(analyzer.type_is_compatible(a, curr_type.value())) { it++; continue; }

        Error::throw_error_and_recover(
            (*it)->span_meta,
            std::format(
                "Expected type {}, got type {}",
                a.dump(), curr_type->dump()
            ).c_str(),
            Error::ErrorCodes::BAD_TYPE
        );
        return nullptr;
    }

    ty = std::make_unique<Type>(handle.ret);
    return std::make_unique<ExprValue>(false, handle.ret);
}

unique_ptr<ExprValue> ExprAddress::analyze(Analyzer& analyzer) {
    analyzer.throw_if_in_global(span_meta);

    var->analyze(analyzer);
    assert(var->get_type() != std::nullopt);
    ty = std::make_unique<Type>(var->get_type().value().ref());

    return std::make_unique<ExprValue>(false, *ty);
}

unique_ptr<ExprValue> ExprDeref::analyze(Analyzer& analyzer) {
    analyzer.throw_if_in_global(span_meta);

    var->analyze(analyzer);
    assert(var->get_type() != std::nullopt);
    ty = std::make_unique<Type>(var->get_type().value().deref());

    return std::make_unique<ExprValue>(false, *ty);
}
