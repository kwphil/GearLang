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
#include <gearlang/sem/val.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/error.hpp>
#include <gearlang/optimizer.hpp>

#include <format>

using namespace Ast::Nodes;
using namespace Sem;

unique_ptr<ExprValue> ExprOp::analyze(Analyzer& analyzer) {
    unique_ptr<ExprValue> lhs = left->analyze(analyzer);
    unique_ptr<ExprValue> rhs = right->analyze(analyzer);

    if(!analyzer.type_is_compatible(lhs->ty, rhs->ty)) {
        assert(left->get_type() != std::nullopt);
        assert(right->get_type() != std::nullopt);

        Error::throw_error_and_recover(
            span_meta,
            std::format(
                "Types do not match. lhs: {}, rhs: {}",
                left->get_type()->dump(), right->get_type()->dump()
            ).c_str(),
            Error::ErrorCodes::BAD_TYPE
        );
        return nullptr;
    }

    // Checking for boolean operators
    if(type >= Type::Gt) {
        ty = std::make_unique<Sem::Type>("bool");
    } else {
        ty = std::make_unique<Sem::Type>(lhs->ty);
    }

    // Implicit cast
    right->set_type(lhs->ty);

    // Optimizer::fold(this);

    return std::make_unique<ExprValue>(lhs->is_const && rhs->is_const, lhs->ty);
}
