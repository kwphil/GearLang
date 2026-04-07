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

#include <vector>
#include <memory>
#include <deque>

#include <gearlang/lex.hpp>
#include <gearlang/ctx.hpp>

namespace Ast::Nodes {
    /// @brief Base class for all AST nodes
    class NodeBase {
    public:
        /// @brief The line number in the source code where the node appears
        const Span span_meta;

        NodeBase(Span span) : span_meta(span) {}
        virtual ~NodeBase() = default;

        /// @brief Parses a node from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed node
        static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);

        /// @brief Parses the node into a string
        /// @return A string representation of the node
        virtual std::string to_string() = 0;

        /// @brief Converts a node into an llvm::Value*. Print a nullptr if not to return anything
        virtual llvm::Value* generate(Context& ctx) = 0;
    };
};

/// @brief generates a node
/// @param node the node to generate
/// @param ctx the current context
void generate_node(Ast::Nodes::NodeBase* node, Context& ctx);

namespace Ast {
    bool check_keyword(std::string target);

    class Program {
    public:
        std::deque<std::unique_ptr<Nodes::NodeBase>> content;
        
    public:
        static Program parse(Lexer::Stream& s);
        inline void add_nodes(std::vector<std::unique_ptr<Nodes::NodeBase>> nodes) {
            for(auto& n : nodes) {
                content.push_front(std::move(n));
            }
        }
        void generate(Context& ctx);
        std::string to_string();
    };
}
