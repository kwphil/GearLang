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

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#include <gearlang/etc.hpp>
#include <gearlang/lex.hpp>

#include <gearlang/sem/analyze.hpp>
#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>

using namespace Sem;
using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;

using namespace Ast::Nodes;

bool Analyzer::analyze_decl_statements(NodeBase* node) {
    if(Stmt* stmt = try_cast<NodeBase, Stmt>(node)) {
        stmt->analyze(*this);
        return true;
    }

    try_cast<NodeBase, Expr>(node)->analyze(*this);
    
    return true;
}

void Analyzer::analyze(vector<unique_ptr<NodeBase>>& nodes) {
    // Steps:
    // Declarations
    // Type checking
    for(auto& node : nodes) {
        analyze_decl_statements(node.get());
    }
}

weak_ptr<Analyzer::Scope> Analyzer::new_scope() {
    shared_ptr<Scope> new_scope = std::make_shared<Scope>();

    active_scopes.push_back(std::move(new_scope));

    return new_scope;
}

void Analyzer::delete_scope() {
    active_scopes.pop_back();
}

std::optional<Variable> Analyzer::decl_lookup(string name) {
    for(auto& curr_scope : active_scopes) {
        auto curr = curr_scope->find(name);

        if(curr != curr_scope->end()) {
            Variable& var = curr->second;

            // Checking if a global was accessed from non-global scope
            if(var.is_global == 2 && !is_global_scope()) {
                var.is_global = 1; 
                // Also let the Let node know
                try_cast<NodeBase, Let>
                    (var.let_stmt)
                    ->is_global = true;
            }

            return var;
        }
    }

    return std::nullopt;
}

void Analyzer::add_variable(string name, Variable var) {
    active_scopes.back()->insert({name, var});
}

bool Analyzer::is_global_scope() {
    return active_scopes.size() == 1;
}

// TODO
bool Analyzer::type_is_compatible(Type lhs, Type rhs) { return true; }
