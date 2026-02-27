#pragma once

#include <optional>
#include <unordered_map>
#include <string>

#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/etc.hpp>
#include "type.hpp"

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using namespace Ast::Nodes;
using namespace Sem;

namespace Sem {
    class Analyzer {
    public:
        typedef std::unordered_map<string, Variable> Scope;

    private: 
        vector<shared_ptr<Scope>> active_scopes;

        bool analyze_decl_statements(NodeBase* node);

    public: 
        Analyzer() { new_scope(); }

        void analyze(std::vector<std::unique_ptr<NodeBase>>& nodes);

        // ---------- DECLARATIONS ---------------
        
        /// @brief Looks up a variable with a given name and scope
        /// @param name the name of the variable
        /// @return the semantic information from the variable
        std::optional<Variable> decl_lookup(string name); 
        /// @brief Pushes a new scope to the stack
        /// @return a pointer to the new scope
        weak_ptr<Scope> new_scope();
        /// @brief Pops the scope off the stack
        void delete_scope();
        /// @brief Adds a variable
        /// @param name the name of the variable
        /// @param var the semantic information from the variable
        void add_variable(string name, Variable var);
        /// @brief Checks if we are in the global scope
        bool is_global_scope();

        // ---------- TYPES ---------------------

        /// @brief Checks if two types are convertible. 
        bool type_is_compatible(Type lhs, Type rhs);
    };
}

constexpr void analyze_nodebase(std::unique_ptr<NodeBase>* node, Analyzer& analyzer) {
    if(Stmt* stmt = cast_from_uptr<NodeBase, Stmt>(node)) {
        stmt->analyze(analyzer);
        return;
    }

    cast_from_uptr<NodeBase, Expr>(node)->analyze(analyzer);
}