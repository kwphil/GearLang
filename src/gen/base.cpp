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
#include <gearlang/ast/branch.hpp>
#include <gearlang/sem/type.hpp>   

#include <gearlang/optimizer.hpp>
#include <gearlang/error.hpp>
#include <gearlang/func.hpp>

// Creates a new scope, generates all the expressions inside the block,
// then pops the scope
llvm::Value* Ast::Nodes::Block::generate(Context& ctx) {
    if(is_dead && is_opt_active(DEAD_CODE_ELIMINATION)) return nullptr;

    for (auto& expr : nodes)
        generate_node(expr.get(), ctx);

    return nullptr;
}

void generate_node(Ast::Nodes::NodeBase* node, Context& ctx) {
    using namespace Ast::Nodes;
    
    if(node->is_dead && is_opt_active(DEAD_CODE_ELIMINATION)) return;
    
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
