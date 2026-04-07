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

#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/etc.hpp>

using namespace Ast::Nodes;
using namespace Sem;

bool If::analyze(Analyzer& analyzer) {
    return analyze_nodebase(&expr, analyzer);
}

bool Else::analyze(Analyzer& analyzer) {
    cond->analyze(analyzer);
    bool et = analyze_nodebase(&expr, analyzer);
    bool ef = analyze_nodebase(&else_expr, analyzer);

    return et && ef;
}

bool Return::analyze(Analyzer& analyzer) {
    expr->analyze(analyzer);
    return true;
}

bool Function::analyze(Analyzer& analyzer) {
    weak_ptr<Analyzer::Scope> fn_scope = analyzer.new_scope();
    vector<Type> arg_handle;

    for(auto& arg : args) { 
        arg->analyze(analyzer);
        auto ty_wrap = arg->get_type();
        assert(ty_wrap != std::nullopt);
        arg_handle.push_back(ty_wrap.value());
    }
    
    if(!analyze_nodebase(&block, analyzer) && ty != Sem::Type("void")) {
        Error::throw_warning(
            block->span_meta,
            "Control reached end of non-void function"
        );
    }

    analyzer.delete_scope();

    Sem::Func handle = {
        .name=name,
        .ret=ty, 
        .args=arg_handle,
        .is_variadic=is_variadic
    };

    analyzer.add_function(name, handle);
    return true;
}

bool ExternFn::analyze(Analyzer& analyzer) {
    vector<Type> arg_handle;
    for(auto& a : args) {
        auto ty_wrap = a->get_type();
        assert(ty_wrap != std::nullopt);
        arg_handle.push_back(ty_wrap.value());
    }
    
    Sem::Func handle = {
        .name=callee,
        .ret=ty,
        .args=arg_handle,
        .is_variadic=is_variadic
    };

    analyzer.add_function(callee, handle);
    return false;
}

bool Block::analyze(Analyzer& analyzer) {
    analyzer.new_scope();
    bool finishes = false;
    bool has_warned = false;
    
    for(auto& node : nodes) {
        if(finishes) {
            node->is_dead = true;
            if(!has_warned) {
                Error::throw_warning(
                    node->span_meta,
                    "Control flow will never reach this statement"
                );
                has_warned = true;
            }
        }
        if(analyze_nodebase(&node, analyzer)) finishes = true;
    }

    analyzer.delete_scope();

    return finishes;
}