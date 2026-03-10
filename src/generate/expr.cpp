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

llvm::Value* get_var(NodeBase* let) {
    if(Let* let_val = try_cast<NodeBase, Let>(let))
        return let_val->var;
    
    return try_cast<NodeBase, Argument>(let)->var;
}

#define CREATE_OP(method, name) \
    out = ctx.builder.method(lhs, rhs, name)

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
llvm::Value* ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);
    llvm::Value* out;

    if(ty->is_float()) {
        switch(type) {
            case Add: CREATE_OP(CreateFAdd, "faddtmp"); break;
            case Sub: CREATE_OP(CreateFSub, "fsubtmp"); break;
            case Mul: CREATE_OP(CreateFMul, "fmultmp"); break;
            case Div: CREATE_OP(CreateFDiv, "fdivtmp"); break;

            case Eq: CREATE_OP(CreateFCmpOEQ, "feqtmp"); break;
            case Ne: CREATE_OP(CreateFCmpONE, "fnetmp"); break;
            case Gt: CREATE_OP(CreateFCmpOGT, "fgttmp"); break;
            case Lt: CREATE_OP(CreateFCmpOLT, "flttmp"); break;
            case Ge: CREATE_OP(CreateFCmpOGE, "fgetmp"); break;
            case Le: CREATE_OP(CreateFCmpOLE, "fletmp"); break;
        }
    } else {
        switch (type) {
            case Add: CREATE_OP(CreateAdd, "iaddtmp"); break;
            case Sub: CREATE_OP(CreateSub, "isubtmp"); break;
            case Mul: CREATE_OP(CreateMul, "imultmp"); break;
            case Div: CREATE_OP(CreateSDiv, "idivtmp"); break;

            case Eq: CREATE_OP(CreateICmpEQ, "feqtmp"); break;
            case Ne: CREATE_OP(CreateICmpNE, "fnetmp"); break;
            case Gt: CREATE_OP(CreateICmpSGT, "fgttmp"); break;
            case Lt: CREATE_OP(CreateICmpSLT, "flttmp"); break;
            case Ge: CREATE_OP(CreateICmpSGE, "fgetmp"); break;
            case Le: CREATE_OP(CreateICmpSLE, "fletmp"); break;
        }
    }

    assert(out);
    return out;
}

llvm::Value* deref(
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

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
llvm::Value* ExprAssign::generate(Context& ctx) {
    llvm::Value* alloca = var->access_alloca(ctx);
    llvm::Value* value = expr->generate(ctx);

    ctx.builder.CreateStore(value, alloca);
    return value;
}

// Gets the function by name, and creates a call for it
// Parses the expressions for each argument and calls it
llvm::Value* ExprCall::generate(Context& ctx) {
    llvm::Function* func = ctx.module->getFunction(callee);

    std::vector<llvm::Value*> arg_values;
    
    for(auto& a : args) {
        arg_values.push_back(a->generate(ctx));
    }

    if(func->getReturnType() == llvm::Type::getVoidTy(ctx.llvmCtx)) { 
        ctx.builder.CreateCall(func, arg_values); 
        return nullptr;
    }

    llvm::Value* val = ctx.builder.CreateCall(func, arg_values, "call");

    return val;
}

llvm::Value* ExprAddress::generate(Context& ctx) {
    // Just return the value because everything is allocated
    // TODO: Later I might optimize this to locate which variables
    // have to alloca and optimize ones that don't out
    return var->access_alloca(ctx);
}

llvm::Value* ExprDeref::generate(Context& ctx) {
    llvm::Value* load = var->generate(ctx);

    return deref(
        ctx,
        load,
        ty.get(),
        var->name,
        ".deref"
    );
}
