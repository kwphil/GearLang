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

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

/// @brief Code generation context
class Context {
public:
    /// @brief LLVM components for code generation
    llvm::LLVMContext llvmCtx;
    /// @brief IR builder for generating LLVM instructions
    llvm::IRBuilder<> builder;
    /// @brief LLVM module representing the generated code
    std::unique_ptr<llvm::Module> module;

    /// @brief Start block for code outside functions
    std::unique_ptr<llvm::BasicBlock*> global_entry; 
    /// @brief Block representing the main function. If main() doesn't exist, this just equals nullptr
    std::unique_ptr<llvm::BasicBlock*> main_entry;
    /// @brief Currently active function being generated
    llvm::Function* current_fn;

    Context()
    : builder(llvmCtx), module(std::make_unique<llvm::Module>("gearlang", llvmCtx)) { }

    /// @brief Create an alloca instruction in the entry block of a function
    /// @param function Function in which to create the alloca
    /// @param name Name of the allocated variable
    /// @param type Type of the allocated variable
    /// @return Pointer to the created alloca instruction
    constexpr llvm::AllocaInst* create_entry_block(
        llvm::Function* function,
        const std::string& name,
        llvm::Type* type
    ) {
        llvm::IRBuilder<> tmpBuilder(
            &function->getEntryBlock(),
            function->getEntryBlock().begin()
        );

        return tmpBuilder.CreateAlloca(type, nullptr, name);
    }

    /// @brief Render the current LLVM module to a string
    constexpr std::string render() {
        std::string out;
        llvm::raw_string_ostream os(out);
        module->print(os, nullptr);
        return out;
    }
};
