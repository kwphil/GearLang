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
                                   
#include <stdexcept>               
#include <memory>                  
                                   
#include <gearlang/ast/base.hpp>   
#include <gearlang/ast/stmt.hpp>   
#include <gearlang/ast/expr.hpp>   
#include <gearlang/sem/type.hpp> 
#include <gearlang/ctx.hpp>

// Generates the if condition, creates blocks for the if and the continuation
// Generates the 'then' block inside the if block
// Jumps to the continuation block afterwards   
llvm::Value* Ast::Nodes::If::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition
    Expr* cond_expr = dynamic_cast<Expr*>(cond.get());
    llvm::Value* condVal = cond_expr->generate(ctx);

    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            condVal->getType(), // Pointers would be handled different so get the raw type
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, then_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);

    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // continuation
    ctx.builder.SetInsertPoint(then_block);

    return nullptr;
}

llvm::Value* Ast::Nodes::Else::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* else_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.else", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition
    Expr* cond_expr = dynamic_cast<Expr*>(cond.get());
    llvm::Value* condVal = cond_expr->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            condVal->getType(),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, else_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // else
    ctx.builder.SetInsertPoint(else_block);
    Expr* else_expr2 = dynamic_cast<Expr*>(expr.get());
    else_expr2->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // end
    ctx.builder.SetInsertPoint(then_block);

    return nullptr;
}
