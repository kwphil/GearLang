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

#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/func.hpp>

using namespace Ast::Nodes;

// Creates the function type. 
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Sets the insert point back to the start block
llvm::Value* Function::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        auto ty_wrap = arg->get_type();
        assert(ty_wrap != std::nullopt);
        param_types.push_back(ty_wrap.value().to_llvm(ctx));
    }

    llvm::Function* fn;
    llvm::BasicBlock* entry;
    unsigned idx = 0;

    fn = declare_func(
        ty.to_llvm(ctx), param_types, identifier.c_str(), ctx, is_variadic, is_public
    );

    entry = llvm::BasicBlock::Create(ctx.llvmCtx, "entry", fn);

    for (auto& llvm_arg : fn->args()) {
        llvm_arg.setName(args[idx++]->name);
    }
    
    ctx.builder.SetInsertPoint(entry);
    ctx.current_fn = fn;

    idx = 0;
    for(auto& arg : fn->args()) {
        auto& ast_arg = args[idx];

        // Alloca
        llvm::Type* arg_ty = arg.getType();
        llvm::AllocaInst* alloca = ctx.create_entry_block(
            fn,
            ast_arg->name,
            arg_ty
        );

        ctx.builder.CreateStore(&arg, alloca);

        ast_arg->var = alloca;

        idx++;
    }

    generate_node(block.get(), ctx);

    if (!fn->back().getTerminator()) {
        if(ty == "void") ctx.builder.CreateRetVoid();
        else ctx.builder.CreateRet(
            llvm::Constant::getNullValue(
                fn->getReturnType()
            )
        );
    }

    ctx.current_fn = nullptr;
    return nullptr;
}

llvm::Value* Ast::Nodes::ExternFn::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            arg->get_type()->to_llvm(ctx)
        );
    }
    
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ty.to_llvm(ctx),
        param_types,
        is_variadic
    );

    llvm::Function::Create(
        fn_type,
        llvm::Function::ExternalLinkage,
        identifier,
        *ctx.module
    );

    return nullptr;
}
