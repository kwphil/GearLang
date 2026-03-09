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

#include <optional>
#include <format>

using namespace Ast::Nodes;
using namespace Sem;
using std::optional;

unique_ptr<ExprValue> ExprCall::analyze(Analyzer& analyzer) {
    optional<Variable> ref = analyzer.decl_lookup(callee);

    if(!ref.has_value()) {
        Error::throw_error(
            span_meta,
            "Function not defined",
            Error::ErrorCodes::FUNCTION_NOT_DEFINED
        );
    }

    Variable retval = ref.value();

    for(auto& arg : args) {
        arg->analyze(analyzer);
    } 

    return std::make_unique<ExprValue>(false, retval.type);
}

unique_ptr<ExprValue> ExprBlock::analyze(Analyzer& analyzer) {
    analyzer.new_scope();
    
    for(auto& node : nodes) {
        analyze_nodebase(&node, analyzer);
    }

    analyzer.delete_scope();

    return nullptr;
}

unique_ptr<ExprValue> ExprAddress::analyze(Analyzer& analyzer) {
    var->analyze(analyzer);
    ty = std::make_unique<Type>(var->get_type().value().ref());

    return std::make_unique<ExprValue>(false, *ty);
}

unique_ptr<ExprValue> ExprDeref::analyze(Analyzer& analyzer) {
    var->analyze(analyzer);
    ty = std::make_unique<Type>(var->get_type().value().deref());

    return std::make_unique<ExprValue>(false, *ty);
}
