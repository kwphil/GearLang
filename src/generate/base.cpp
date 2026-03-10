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

#include <gearlang/error.hpp>
#include <gearlang/func.hpp>

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
            llvm::GlobalValue::ExternalLinkage,
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

// Creates a new scope, generates all the expressions inside the block,
// then pops the scope
llvm::Value* Ast::Nodes::ExprBlock::generate(Context& ctx) {
    for (auto& expr : nodes)
        generate_node(expr.get(), ctx);

    return nullptr;
}

llvm::Value* Ast::Nodes::Return::generate(Context& ctx) {
    Expr* exp = dynamic_cast<Expr*>(expr.get());
    llvm::Value* retVal = exp->generate(ctx);
    llvm::Type* fn_return = ctx.current_fn->getReturnType();

    llvm::Value* ret_ir = retVal;
    if(exp->get_type()->to_llvm(ctx) != fn_return) {
        ret_ir = ctx.builder.CreateIntCast(retVal, fn_return, true);
    }

    ctx.builder.CreateRet(ret_ir);

    return nullptr;
}

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

void generate_node(Ast::Nodes::NodeBase* node, Context& ctx) {
    using namespace Ast::Nodes;
    
    if (auto* stmt = dynamic_cast<Stmt*>(node)) {
        stmt->generate(ctx);
    } else if (auto* expr = dynamic_cast<Expr*>(node)) {
        expr->generate(ctx);
    } else {
        throw std::runtime_error("invalid node");
    }
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& node : content) {
        generate_node(node.get(), ctx);
    }
}
