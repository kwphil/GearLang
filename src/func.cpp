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

#include <gearlang/func.hpp>
#include <gearlang/ctx.hpp>

llvm::Function* declare_func(
    llvm::Type* ret_type,
    llvm::ArrayRef<llvm::Type*> args,
    const char* name, Context& ctx,
    bool variadic
) {
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ret_type,
        args,
        variadic
    );

    return llvm::Function::Create(
        fn_type,
        llvm::GlobalValue::ExternalLinkage,
        name, *ctx.module
    );
}

void call_exit(Context& ctx, llvm::Value* retVal) {
    static llvm::Function* exit_fn;

    if(!exit_fn) { // If exit hasn't been created yet
        exit_fn = declare_func( // Setting up exit call
            llvm::Type::getVoidTy(ctx.llvmCtx),
            { llvm::Type::getInt32Ty(ctx.llvmCtx) },
            "exit", ctx, false
        );
    }

    ctx.builder.CreateCall(exit_fn, { retVal });
    ctx.builder.CreateUnreachable();
}
