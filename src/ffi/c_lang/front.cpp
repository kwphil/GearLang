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

#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <gearlang/ast/base.hpp>
#include <gearlang/ffi/c.hpp>
#include <gearlang/error.hpp>

#include "visitor.hpp"

#include <vector>
#include <memory>

using std::vector;
using std::unique_ptr;
using namespace Ast::Nodes;
using namespace clang;

const vector<string> C_FFI_ARGS = {
    "-I.",
    "-std=c99"
};

class CFfiConsumer : public ASTConsumer {
public:
    CFfiConsumer(C_Ffi& manager) : visitor(manager) { }

    void HandleTranslationUnit(ASTContext& context) override {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    CAstVisitor visitor;
};

vector<unique_ptr<NodeBase>> C_Ffi::compile_headers() {
    vector<unique_ptr<ASTUnit>> asts;

    auto ast = tooling::buildASTFromCodeWithArgs(
        src.str(), 
        C_FFI_ARGS,
        "ffiwrap.c"
    );

    if (ast) {
        CFfiConsumer consumer(*this);
        consumer.HandleTranslationUnit(ast->getASTContext());
        asts.push_back(std::move(ast));
    } else {
        Error::throw_error(
            {0, 0, 0, 0},
            "Unable to parse C header",
            Error::ErrorCodes::INVALID_AST
        );
    }

    type_cache.clear();

    return std::move(nodes);
}

vector<std::string> splitStringstream(const string& input, char delimiter) {
    vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

void C_Ffi::add_header(string filename) {
    auto split = splitStringstream(filename, ':');
    if (split.size() == 1 || split[0] == "loc") {
        add_header_to_file(format("#include \"{}\"", split.back()));
    } else if (split[0] == "sys") {
        add_header_to_file(format("#include <{}>", split[1]));
    } else {
        assert(false && "Unexpected header path!");
    }
}