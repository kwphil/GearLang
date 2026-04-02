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

#include <clang/AST/Type.h>
#include <clang/AST/Decl.h>

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <gearlang/ast/base.hpp>
#include <gearlang/sem/type.hpp>
#include "core.hpp"

using std::vector;
using std::unique_ptr;
using std::ofstream;
using std::string;
using std::unordered_map;

class C_Ffi : public Ffi {
private:
    vector<unique_ptr<Ast::Nodes::NodeBase>> nodes;
    unordered_map<const clang::Type*, shared_ptr<Sem::Type>> type_cache;

public:
    virtual void add_header(string filename) override;
    virtual vector<unique_ptr<Ast::Nodes::NodeBase>> compile_headers() override;

    shared_ptr<Sem::Type> c_to_gear_ty(clang::QualType* qt);
    shared_ptr<Sem::Type> c_record_to_gear(const clang::RecordDecl* ty);
    inline void add_node(unique_ptr<Ast::Nodes::NodeBase> node) { nodes.push_back(std::move(node)); }
};