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

#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <gearlang/ast/expr.hpp>
#include <gearlang/sem/type.hpp>

// Just generates an int constant and returns it
unique_ptr<Value> Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return std::make_unique<Value>(
        llvm::ConstantInt::get(
            ty->to_llvm(ctx),
            this->value,
            true
        ),
        ty->to_llvm(ctx),
        false
    );
}

// TODO
unique_ptr<Value> Ast::Nodes::ExprLitFloat::generate(Context& ctx) {
    return std::make_unique<Value>(
        llvm::ConstantFP::get(
            ty->to_llvm(ctx),
            this->value
        ),
        ty->to_llvm(ctx),
        false
    );
};

// Converts a string into an array of constant i8s
// Creates a constant array using that
// Returns a pointer to the string
unique_ptr<Value> Ast::Nodes::ExprLitString::generate(Context& ctx) {
    std::vector<llvm::Constant*> chars(string.size());
    llvm::Type* i8 = llvm::Type::getInt8Ty(ctx.llvmCtx);
    llvm::ArrayType* arr_i8 = llvm::ArrayType::get(i8, chars.size());

    for(long unsigned int i = 0; i < string.size(); i++) {
        chars[i] = llvm::ConstantInt::get(i8, string[i]);
    }

    auto array = llvm::ConstantArray::get(
        arr_i8, 
        chars
    );

    llvm::Value* val = new llvm::GlobalVariable(
        *ctx.module,
        arr_i8,
        true,
        llvm::GlobalValue::ExternalLinkage,
        array,
        ".str"
    );

    return std::make_unique<Value>(
        val,
        arr_i8,
        false
    );
}
