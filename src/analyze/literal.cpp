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
#include <gearlang/sem/type.hpp>
#include <gearlang/sem/val.hpp>

using namespace Ast::Nodes;
using Sem::ExprValue;
using Sem::Type;

unique_ptr<ExprValue> ExprLitInt::analyze(Sem::Analyzer& analyzer) {
    Type raw_ty;

    if(value <= 0xff) raw_ty = Type("i8");
    else if(value <= 0xffff) raw_ty = Type("i16");
    else if(value <= 0xffffffff) raw_ty = Type("i32");
    else raw_ty = Type("i64");

    ty = std::make_unique<Type>(raw_ty);

    return std::make_unique<ExprValue>(true, *ty);
}

unique_ptr<ExprValue> ExprLitFloat::analyze(Sem::Analyzer& analyzer) {
    ty = std::make_unique<Type>("f32");
    return std::make_unique<ExprValue>(true, *ty);
}

unique_ptr<ExprValue> ExprLitString::analyze(Sem::Analyzer& analyzer) {
    ty = std::make_unique<Type>("char^");
    return std::make_unique<ExprValue>(true, *ty);
}