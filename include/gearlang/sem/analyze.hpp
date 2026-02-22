#pragma once

#include <optional>
#include <unordered_map>
#include <string>

#include <gearlang/ast/base.hpp>
#include <gearlang/lex.hpp>
#include "type.hpp"

using std::string;
using std::vector;
using Ast::Nodes::NodeBase;

namespace Sem {
    class Analyzer {
    public:
        typedef std::unordered_map<string, Variable> Scope;

    private: 
        // Nodes that don't get removed are moved here for generation
        vector<std::unique_ptr<NodeBase>> analyzed_nodes;

        Scope* global_scope;
        vector<Scope*> active_scopes;
        Scope* new_scope();
        void delete_scope();

        bool analyze_decl_statements(std::unique_ptr<NodeBase>* node);

    public: 
        Analyzer()
        : active_scopes({ new Scope() }) { }

        void analyze(std::vector<std::unique_ptr<NodeBase>>& nodes);

        // ---------- DECLARATIONS ---------------
        
        /// @brief Looks up a variable with a given name and scope
        /// @param name the name of the variable
        /// @return the semantic information from the variable
        std::optional<Variable> decl_lookup(string name); 
        /// @brief Adds a variable
        /// @param name the name of the variable
        /// @param var the semantic information from the variable
        void add_variable(string name, Variable var);
        /// @brief Checks if we are in the global scope
        bool is_global_scope();

        // ---------- TYPES ---------------------

        /// @brief Checks if two types are convertible. 
        bool type_is_compatible(Type lhs, Type rhs);

        // ---------- ETC ------------------------

        /// @brief Pushes a new node to be generated.
        void add_node(std::unique_ptr<NodeBase> node);
    };
}