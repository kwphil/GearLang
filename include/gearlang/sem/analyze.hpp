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

#include <optional>
#include <unordered_map>
#include <string>
#include <deque>
#include <iostream>

#include <gearlang/ast/base.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/expr.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/etc.hpp>
#include "type.hpp"
#include "analyzer_debug.hpp"

using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::unordered_map;
using std::optional;
using namespace Ast::Nodes;
using namespace Sem;

namespace Sem {
    class Analyzer {
    public:
        typedef std::unordered_map<string, Variable> Scope;

    private: 
        Logger logger;
        vector<shared_ptr<Scope>> active_scopes;
        unordered_map<string, Func> function_list; 

        bool analyze_decl_statements(NodeBase* node);

    public: 
        bool dump_self;

        Analyzer(bool dump) : dump_self(dump) { new_scope(); }

        void analyze(std::deque<std::unique_ptr<NodeBase>>& nodes);

        // ---------- DUMP ANALYZER --------------

        void trace(
            const std::unordered_map<std::string, std::string>& data,
            Error::ErrorCodes code,
            Span span
        ) {
            if (!dump_self) return;
            auto enriched = data;

            logger.log(enriched, code, span);
        }

        void dump() { if(dump_self) std::cout << logger.to_json() << std::endl; }

        // ---------- DECLARATIONS ---------------
        
        /// @brief Looks up a variable with a given name and scope
        /// @param name the name of the variable
        /// @return the semantic information from the variable
        optional<Variable> decl_lookup(string name); 
        /// @brief Pushes a new scope to the stack
        /// @return a pointer to the new scope
        weak_ptr<Scope> new_scope();
        /// @brief Pops the scope off the stack
        void delete_scope();
        /// @brief Adds a variable
        /// @param name the name of the variable
        /// @param var the semantic information from the variable
        void add_variable(string name, Variable var);
        /// @brief Addes a function handle
        /// @param name the name of the function
        /// @param fn the semantic info from the func
        inline void add_function(string name, Func fn) {
            function_list.insert({ name, fn }); 
        }
        /// @brief Looks up a function by a given name
        /// @param name the name of the function
        /// @returns an optional Func
        inline optional<Func> lookup_func(string name) {
            auto it = function_list.find(name);

            if(it == function_list.end()) return std::nullopt;
            return it->second;
        }
        /// @brief Checks if we are in the global scope
        bool is_global_scope();

        // ---------- TYPES ---------------------

        /// @brief Checks if two types are convertible. 
        bool type_is_compatible(Type lhs, Type rhs);
    };
}

constexpr bool analyze_nodebase(std::unique_ptr<NodeBase>* node, Analyzer& analyzer) {
    if(Stmt* stmt = cast_from_uptr<NodeBase, Stmt>(node)) {
        return stmt->analyze(analyzer);
    }

    cast_from_uptr<NodeBase, Expr>(node)->analyze(analyzer);
    return false;
}
