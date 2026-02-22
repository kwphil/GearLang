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

using Ast::Nodes::NodeBase;
using Ast::Nodes::Let;

bool Analyzer::analyze_decl_statements(std::unique_ptr<NodeBase>* node) {
    Let* let = try_cast<NodeBase, Let>(node->get());

    if(let) {
        let->analyze(*this);

        // Need to get the Variable we created
        auto& var = active_scopes.back()->find(let->get_name())->second;

        // Don't need this for now, access with the vector instead
        add_node(std::move(*node));

        // Now we can set the index if need to be updated
        var.node_index = analyzed_nodes.size()-1;

        return true;
    }

    return false;
}

#include <iostream>

void Analyzer::analyze(vector<unique_ptr<NodeBase>>& nodes) {
    // Steps:
    // Declarations
    // Type checking
    for(auto& node : nodes) {
        if(
            analyze_decl_statements(&node)
        ) {

        }
    }

    for(auto& scope : active_scopes) {
        for(auto& pair : *scope) {
            auto& var = pair.second;
            std::cout << var.name << ": type=" << var.type.dump() << ", global=" << (int)(var.is_global) << std::endl;;
        }
    }
}

Analyzer::Scope* Analyzer::new_scope() {
    Scope* new_scope = new Scope();

    active_scopes.push_back(new_scope);

    return new_scope;
}

void Analyzer::delete_scope() {
    delete active_scopes.back();
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
                cast_from_uptr<NodeBase, Let>
                    (&analyzed_nodes[var.node_index])
                    ->is_global = true;
            }

            return var;
        }
    }

    return std::nullopt;
}

void Analyzer::add_variable(string name, Variable var) {
    auto bottom_scope = active_scopes.back();
    (*bottom_scope)[name] = var;
}

bool Analyzer::is_global_scope() {
    return active_scopes.size() == 1;
}

// TODO
bool Analyzer::type_is_compatible(Type lhs, Type rhs) { return false; }

void Analyzer::add_node(std::unique_ptr<NodeBase> node) {
    analyzed_nodes.push_back(std::move(node));
}