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
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/ast/vars.hpp>

#include <gearlang/error.hpp>
#include <gearlang/etc.hpp>

using namespace Ast::Nodes;

inline llvm::Value* deref(
    Context& ctx, 
    llvm::Value* ptr, 
    Sem::Type* type,
    const std::string& name,
    const char* suffix
) {
    return ctx.builder.CreateLoad(
        type->to_llvm(ctx),
        ptr,
        name + suffix
    );
}

llvm::Value* get_var(NodeBase* let) {
    if(Let* let_val = try_cast<NodeBase, Let>(let))
        return let_val->var;
    
    return try_cast<NodeBase, Argument>(let)->var;
}


// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise Checks if it is a global and returns it
llvm::Value* ExprVar::generate(Context& ctx) {
    return deref(
        ctx, access_alloca(ctx),
        ty.get(), name, ".load"
    );
}

llvm::Value* ExprVar::access_alloca(Context& _ctx) {
    return get_var(let);
}

llvm::Value* ExprStructParam::generate(Context& ctx) {
    llvm::Type* ty_ll = ty->struct_param_ty(index).to_llvm(ctx);

    return ctx.builder.CreateLoad(ty_ll, access_alloca(ctx), struct_name + "." + name + ".load");
}

// Returns the address calculated from the GEP
llvm::Value* ExprStructParam::access_alloca(Context& ctx) {
    llvm::Type* struct_ll = ty->get_llvm_struct();

    return ctx.builder.CreateStructGEP(
        struct_ll, get_var(let), index+1
    );
}
