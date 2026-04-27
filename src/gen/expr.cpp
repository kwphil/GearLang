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
#include <gearlang/ast/lit.hpp>

#include <gearlang/error.hpp>
#include <gearlang/etc.hpp>

using namespace Ast::Nodes;
using enum Sem::Type::PrimType;

#define CREATE_OP(method) \
    out = ctx.builder.method(lhs, rhs)

inline llvm::Value* cast_float(llvm::Value* val, Sem::Type ty, Context& ctx) {
    if(ty.is_float()) {
        llvm::Type* new_ty;

        if(ty.prim_type == F32) {
            new_ty = Sem::Type("u32").to_llvm(ctx);
        } else if(ty.prim_type == F64) {
            new_ty = Sem::Type("u64").to_llvm(ctx);
        } else {
            throw std::runtime_error("Unexpected float type");
        }

        return ctx.builder.CreateFPToUI(val, new_ty);
    }

    // No need to cast
    return val;
}

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
llvm::Value* ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs;
    if(right) { // Unary operations won't have an rhs
        rhs = right->generate(ctx);
    }
    llvm::Value* out;

    // Bitwise operations
    if(type >= And && type <= Sizeof) {
        llvm::Value* l = cast_float(lhs, *left->get_type(), ctx);
        llvm::Value* r;
        if(right)
            r = cast_float(rhs, *right->get_type(), ctx);

        switch(type) {
            case And: out = ctx.builder.CreateAnd(l, r);  break;
            case Or:  out = ctx.builder.CreateOr(l, r);   break;
            case Xor: out = ctx.builder.CreateXor(l, r);  break;
            case Not: out = ctx.builder.CreateNot(l);     break;
            case Sizeof: {
                auto ty = llvm::Type::getInt8Ty(ctx.llvmCtx);

                out = llvm::ConstantInt::get(ty,
                    ctx.layout_query.getTypeAllocSize(ty) 
                );

                break;
            }
            default: throw std::runtime_error("Unknown operation");
        }

        assert(out);
        return out;
    }

    if(ty->is_float()) {
        switch(type) {
            case Add: CREATE_OP(CreateFAdd); break;
            case Sub: CREATE_OP(CreateFSub); break;
            case Mul: CREATE_OP(CreateFMul); break;
            case Div: CREATE_OP(CreateFDiv); break;

            case Eq: CREATE_OP(CreateFCmpOEQ); break;
            case Ne: CREATE_OP(CreateFCmpONE); break;
            case Gt: CREATE_OP(CreateFCmpOGT); break;
            case Lt: CREATE_OP(CreateFCmpOLT); break;
            case Ge: CREATE_OP(CreateFCmpOGE); break;
            case Le: CREATE_OP(CreateFCmpOLE); break;

            default: break;
        }
    } else {
        switch (type) {
            case Add: CREATE_OP(CreateAdd); break;
            case Sub: CREATE_OP(CreateSub); break;
            case Mul: CREATE_OP(CreateMul); break;
            case Div: CREATE_OP(CreateSDiv); break;

            case Eq: CREATE_OP(CreateICmpEQ); break;
            case Ne: CREATE_OP(CreateICmpNE); break;
            case Gt: CREATE_OP(CreateICmpSGT); break;
            case Lt: CREATE_OP(CreateICmpSLT); break;
            case Ge: CREATE_OP(CreateICmpSGE); break;
            case Le: CREATE_OP(CreateICmpSLE); break;

            default: break;
        }
    }

    assert(out);
    return out;
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
    llvm::Function* func = ctx.module->getFunction(identifier);

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
