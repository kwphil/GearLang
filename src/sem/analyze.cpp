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

    for(auto& scope : active_scopes) {
        for(auto& pair : *scope) {
            auto& var = pair.second;
        }
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
