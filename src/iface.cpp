#include <gearlang/ast/base.hpp>
#include <gearlang/ast/func.hpp>
#include <gearlang/sem/analyze.hpp>
#include <gearlang/error.hpp>

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <format>

namespace fs = std::filesystem;
using namespace Ast::Nodes;
using std::vector;
using std::string;

vector<unique_ptr<NodeBase>> parse_module_file(string mgr_path, string mod) {
    std::ifstream mgr(mgr_path);
    vector<unique_ptr<NodeBase>> nodes;
    string buf;
    string file;

    while(std::getline(mgr, buf)) {
        // Comments
        buf.erase(0, buf.find_first_not_of(" "));

        if(buf.empty()) continue;

        if(buf[0] == ';') {
            continue;
        }

        if(buf[0] == ':') {
            auto command = split_string(buf, ' ')[0];
            if(command == ":FILE") {
                file = command;
            } else {
                std::cerr << "Unknown command: " << command << std::endl;
            }

            continue;
        }

        Lexer::Stream s = Lexer::tokenize_by_string(buf, file);
        unique_ptr<NodeBase> node = NodeBase::parse(s); 
        node->span_meta = { 
            file, 
            stoul(s.pop()->content), stoul(s.pop()->content), 
            stoul(s.pop()->content), stoul(s.pop()->content) 
        };

        if(Let* let = cast_to<Let>(node.get())) {
            let->prefix(mod);
        }

        if(ExternFn* fn = cast_to<ExternFn>(node.get())) {
            fn->prefix(mod);
        }

        nodes.push_back(std::move(node));
    }

    return nodes;
}

/// @brief Either generates a modgear file, or parses one given the source path
/// @param mod The name of the module (not the file)
/// @param span Where span was introduced
/// @param build_dir the absolute path for building the directory
/// @returns A list of NodeBases to insert into the global AST list
vector<unique_ptr<NodeBase>> load_module(
    string& mod, Span& span, string& build_dir, string& src_path
) {
    // First check to see if an mgr exists
    string mgr_path = build_dir + '/' + mod + ".mgr";

    // mgr exists, so parse
    if(fs::exists(mgr_path)) {
        return parse_module_file(mgr_path, mod);
    }

    // mgr doesn't excists, generate one
    std::ofstream mgr(mgr_path);

    string src_file;

    for(const auto& entry : fs::directory_iterator(src_path)) { 
        string filename = entry.path().filename().string();
        if(filename.compare(0, mod.length(), mod) == 0) {
            if(src_file != "") {
                Error::throw_error(
                    span, (string("Found multiple candidates for module: ") + mod).c_str(), 
                    Error::ErrorCodes::INVALID_MODULE
                );
            }
            
            src_file = entry.path();
        }
    }

    if(src_file == "") {
        Error::throw_error(
            span, (string("Candidate for module not found: ") + mod).c_str(), 
            Error::ErrorCodes::INVALID_MODULE
        );
    }

    mgr << ":FILE " << src_file << '\n';

    // Build the AST tree and ensure correctness.
    Lexer::Stream s = Lexer::tokenize(src_file);
    auto root = Ast::Program::parse(s);
    s.reset();

    while(s.has()) {
        if(s.peek()->content == "export") {
            mgr << "extern ";
            
            Span span = s.pop()->span;
 
            do {
                mgr << s.pop()->content << " ";
            } while( // All delimiters I don't want
                s.peek()->type != Lexer::Type::Semi && 
                s.peek()->type != Lexer::Type::BraceOpen && 
                s.peek()->content != "="
            );
            span.end = s.peek()->span.end;
            s.pop();

            mgr << std::format("; {} {} {} {}\n\n", 
                span.line, span.col, span.start, span.end
            );
        } else s.pop();
    }

    mgr.close();
    // Now that the file exists parse the module
    return load_module(mod, span, build_dir, src_path);
}