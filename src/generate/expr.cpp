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

#include <gearlang/error.hpp>
#include <gearlang/etc.hpp>

using namespace Ast::Nodes;

llvm::Value* get_var(NodeBase* let) {
    if(Let* let_val = try_cast<NodeBase, Let>(let))
        return let_val->var;
    
    return try_cast<NodeBase, Argument>(let)->var;
}

#define CREATE_OP(method, name) \
    out = ctx.builder.method(lhs->ir, rhs->ir, name)

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
unique_ptr<Value> ExprOp::generate(Context& ctx) {
    unique_ptr<Value> lhs = left->generate(ctx);
    unique_ptr<Value> rhs = right->generate(ctx);
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
            case Add: out = ctx.builder.CreateAdd(lhs->ir, rhs->ir, "iaddtmp"); break;
            case Sub: out = ctx.builder.CreateSub(lhs->ir, rhs->ir, "isubtmp"); break;
            case Mul: out = ctx.builder.CreateMul(lhs->ir, rhs->ir, "imultmp"); break;
            case Div: out = ctx.builder.CreateSDiv(lhs->ir, rhs->ir, "idivtmp"); break;

            case Eq: CREATE_OP(CreateICmpEQ, "feqtmp"); break;
            case Ne: CREATE_OP(CreateICmpNE, "fnetmp"); break;
            case Gt: CREATE_OP(CreateICmpSGT, "fgttmp"); break;
            case Lt: CREATE_OP(CreateICmpSLT, "flttmp"); break;
            case Ge: CREATE_OP(CreateICmpSGE, "fgetmp"); break;
            case Le: CREATE_OP(CreateICmpSLE, "fletmp"); break;
        }
    }

    if(out) {
        return std::make_unique<Value>(
            out,
            ty->to_llvm(ctx),
            lhs->addr
        );
    }

    assert(false && "You shouldn't be here!!");
}

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise Checks if it is a global and returns it
unique_ptr<Value> ExprVar::generate(Context& ctx) {
    // Link to the declaration statement
    llvm::Value* var = get_var(let);

    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        llvm::Value* ir = ctx.builder.CreateLoad(
            ty->to_llvm(ctx),
            gv,
            name + ".load"
        );

        return std::make_unique<Value>(
            ir,
            ty->to_llvm(ctx),
            ty->pointer_level()
        );
    }

    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(var);

    llvm::Value* ir = ctx.builder.CreateLoad(
        alloca->getAllocatedType(),
        var,
        name + ".load"
    );

    return std::make_unique<Value>(
        ir,
        ty->to_llvm(ctx),
        ty->pointer_level()
    );
}

unique_ptr<Value> ExprStructParam::generate(Context& ctx) {
    llvm::Value* param_ptr = ctx.builder.CreateStructGEP(
        ty->get_llvm_struct(), get_var(let), index
    );

    Sem::Type param_ty = ty->struct_param_ty(index);
    llvm::Value* var = ctx.builder.CreateLoad(param_ty.to_llvm(ctx), param_ptr);

    std::make_unique<Value>(
        var,
        param_ty.to_llvm(ctx),
        param_ty.pointer_level()
    );
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
unique_ptr<Value> ExprAssign::generate(Context& ctx) {
    llvm::Value* var = get_var(let);

    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    
    unique_ptr<Value> value = expr2->generate(ctx);

    ctx.builder.CreateStore(value->ir, var);

    return value;
}

// Gets the function by name, and creates a call for it
// Parses the expressions for each argument and calls it
unique_ptr<Value> ExprCall::generate(Context& ctx) {
    llvm::Function* func = ctx.module->getFunction(callee);

    std::vector<llvm::Value*> arg_values;
    
    for(auto& a : args) {
        arg_values.push_back(a->generate(ctx)->ir);
    }

    if(func->getReturnType() == llvm::Type::getVoidTy(ctx.llvmCtx)) { 
        ctx.builder.CreateCall(func, arg_values); 
        return nullptr;
    }

    llvm::Value* val = ctx.builder.CreateCall(func, arg_values, "call");

    return std::make_unique<Value>(
        val,
        val->getType(),
        0
    );
}

unique_ptr<Value> ExprAddress::generate(Context& ctx) {
    // Just return the value because everything is allocated
    // TODO: Later I might optimize this to locate which variables
    // have to alloca and optimize ones that don't out
    return std::make_unique<Value>(
        get_var(let),
        ty->to_llvm(ctx),
        ty->ref().pointer_level()
    );
}

llvm::Value* deref(
    Context& ctx, 
    llvm::Value* ptr, 
    llvm::Type* type,
    std::string&& name,
    const char* suffix, 
    int line_number
) {
    // First try if it is a global
    if(auto* gptr = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
        return ctx.builder.CreateLoad(
            type,
            gptr,
            name + suffix
        );
    }

    return ctx.builder.CreateLoad(
        type,
        ptr,
        name + suffix
    );
}

unique_ptr<Value> ExprDeref::generate(Context& ctx) {
    llvm::Value* var = get_var(let);

    // Load the variable twice and return
    // Once to get the pointer
    // Twice to get the dereferenced data

    llvm::Value* load = deref(
        ctx,
        var,
        ty->to_llvm(ctx),
        var->getName().str(),
        ".load",
        span_meta.line
    );

    llvm::Value* deref_var = deref(
        ctx,
        load,
        ty->to_llvm(ctx),
        var->getName().str(),
        ".deref",
        span_meta.line
    );

    return std::make_unique<Value>(
        deref_var,
        ty->to_llvm(ctx),
        ty->pointer_level()-1
    );
}
