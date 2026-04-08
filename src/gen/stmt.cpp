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
#include <llvm/IR/Function.h>      
#include <llvm/IR/Module.h>        
#include <llvm/IR/BasicBlock.h>    
#include <llvm/IR/Constants.h>     
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>          
                                   
#include <memory>                  
                                   
#include <gearlang/ast/base.hpp>   
#include <gearlang/ast/stmt.hpp>   
#include <gearlang/ast/expr.hpp>   
#include <gearlang/sem/type.hpp>   

#include <gearlang/error.hpp>
#include <gearlang/func.hpp>

llvm::Value* Ast::Nodes::Return::generate(Context& ctx) {
    Expr* exp = dynamic_cast<Expr*>(expr.get());
    llvm::Value* retVal = exp->generate(ctx);
    llvm::Type* fn_return = ctx.current_fn->getReturnType();

    llvm::Value* ret_ir = retVal;
    assert(exp->get_type() != std::nullopt);
    if(exp->get_type()->to_llvm(ctx) != fn_return) {
        ret_ir = ctx.builder.CreateIntCast(retVal, fn_return, true);
    }

    ctx.builder.CreateRet(ret_ir);

    return nullptr;
}

// Checks if the variable already exists
// If it does, it throws an error and quits
// Otherwise, it creates the variable in the current scope
// If in the global scope, it creates a global variable
llvm::Value* Ast::Nodes::Let::generate(Context& ctx) {
    Expr* rvalue = dynamic_cast<Expr*>(expr.get());
    llvm::Value* initVal;

    if(rvalue) {
        initVal = rvalue->generate(ctx);
    }

    if (is_global) {
        // Creating a placeholder and then assigning the value
        llvm::Constant* placeholder = 
            llvm::Constant::getNullValue(ty->to_llvm(ctx));
        
        // Create the variable
        llvm::GlobalVariable* var = new llvm::GlobalVariable(
            *(ctx.module),
            ty->to_llvm(ctx),
            false,
            is_public 
            ? llvm::GlobalValue::ExternalLinkage
            : llvm::GlobalValue::PrivateLinkage,
            placeholder,
            target
        );

        if(rvalue) {
            ctx.builder.CreateStore(initVal, var);
        }

        this->var = var;

        return nullptr;
    }

    llvm::Function* fn = ctx.current_fn;
    llvm::AllocaInst* alloca =
        ctx.create_entry_block(fn, target, ty->to_llvm(ctx));

    var = alloca;

    if(rvalue) {
        ctx.builder.CreateStore(initVal, alloca);
    }

    return nullptr;
}

llvm::Value* Ast::Nodes::While::generate(Context& ctx) {
    llvm::BasicBlock* loop_cond = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "while.cond", ctx.current_fn);
    llvm::BasicBlock* loop_body = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "while.body", ctx.current_fn);
    llvm::BasicBlock* after_loop = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "while.end", ctx.current_fn);

    ctx.builder.CreateBr(loop_cond);

    ctx.builder.SetInsertPoint(loop_cond);

    llvm::Value* cond_val = cond->generate(ctx);

    llvm::Value* condv = ctx.builder.CreateICmpNE(
        cond_val,
        llvm::ConstantInt::get(
            cond_val->getType(),
            0,
            true
        ),
        "wcond"
    );

    ctx.builder.CreateCondBr(condv, loop_body, after_loop);
    ctx.builder.SetInsertPoint(loop_body);
    
    code->generate(ctx);

    ctx.builder.CreateBr(loop_cond); 
    ctx.builder.SetInsertPoint(after_loop);

    return nullptr;
}

llvm::Value* Ast::Nodes::Do::generate(Context& ctx) {
    llvm::BasicBlock* loop_body = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "do.body", ctx.current_fn);
    llvm::BasicBlock* loop_cond = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "do.cond", ctx.current_fn);
    llvm::BasicBlock* after_loop = 
        llvm::BasicBlock::Create(ctx.llvmCtx, "do.end", ctx.current_fn);

    ctx.builder.CreateBr(loop_body);
    ctx.builder.SetInsertPoint(loop_body);

    code->generate(ctx);

    ctx.builder.CreateBr(loop_body);
    ctx.builder.SetInsertPoint(loop_cond);

    llvm::Value* cond_val = cond->generate(ctx);
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        cond_val,
        llvm::ConstantInt::get(
            cond_val->getType(),
            0,
            true
        ),
        "dcond"
    );

    ctx.builder.CreateCondBr(condv, loop_body, after_loop);
    ctx.builder.SetInsertPoint(loop_body);
    
    ctx.builder.CreateBr(loop_body); 
    ctx.builder.SetInsertPoint(after_loop);

    return nullptr;
}