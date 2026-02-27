#pragma once

#include <vector>
#include <memory>

#include <gearlang/lex.hpp>
#include <gearlang/ctx.hpp>

namespace Ast::Nodes {
    /// @brief Base class for all AST nodes
    class NodeBase {
    public:
        /// @brief The line number in the source code where the node appears
        const int line_number;

        NodeBase(int line_number) : line_number(line_number) {}
        virtual ~NodeBase() = default;

        /// @brief Parses a node from the lexer stream
        /// @param s The lexer stream to parse from
        /// @return A unique pointer to the parsed node
        static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);
    };
};

/// @brief generates a node
/// @param node the node to generate
/// @param ctx the current context
void generate_node(Ast::Nodes::NodeBase* node, Context& ctx);

namespace Ast {
    class Program {
    public:
        std::vector<std::unique_ptr<Nodes::NodeBase>> content;
        
    public:
        static Program parse(Lexer::Stream& s);

        void generate(Context& ctx);
    };
}
