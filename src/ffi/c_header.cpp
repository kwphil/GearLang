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

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/MemoryBuffer.h>

#include <gearlang/ffi/c.hpp>
#include <gearlang/ast/func.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <format>

using std::unordered_map;
using std::vector;
using std::unique_ptr;
using std::string;
using std::format;

vector<unique_ptr<Ast::Nodes::NodeBase>> nodes;

const vector<string> C_FFI_ARGS = {
    "-I.",
    "-std=c99"
};

Type c_builtin_to_gear(const clang::BuiltinType* ty) {
    using clang::BuiltinType;
    using Sem::Type;

    switch(ty->getKind()) {
        case BuiltinType::Int: return Type("i32");
        case BuiltinType::Float: return Type("f32");
        case BuiltinType::Double: return Type("f64");
        case BuiltinType::Bool: return Type("bool");
        case BuiltinType::Long: return Type("i64");
        case BuiltinType::LongLong: return Type("i128");
        case BuiltinType::Char_S: return Type("i8");
        case BuiltinType::Void: return Type("void");
        case BuiltinType::ULong: return Type("u64");
        case BuiltinType::UInt: return Type("u32");

        default: 
            clang::LangOptions LO;  // default language options
            clang::PrintingPolicy policy(LO);
            throw std::runtime_error(ty->getName(policy).str()); 
    }
}

Type c_to_gear_ty(clang::QualType* qt);

static unordered_map<const clang::Type*, Sem::Type> type_cache;
static int anon_struct = 0;

Type c_record_to_gear(const clang::RecordDecl* ty) {
    string name = ty->getNameAsString();

    if(name == "") {
        name = "__GEAR_anonymous_struct_";
        name += std::to_string(anon_struct++);
        std::cout << name << std::endl;
    }

    string parse_str = "struct ";
    parse_str += name;
    parse_str += " {";

    for(auto* field : ty->fields()) {
        parse_str += field->getNameAsString() + ' ';
        auto field_ty = field->getType();
        parse_str += c_to_gear_ty(&field_ty).dump() + ';';
    }

    parse_str += "}";

    Ast::Nodes::Struct strct(ty->getNameAsString(), Type(parse_str), { 0, 0, 0, 0 });
    return Type(parse_str);
}

Type c_to_gear_ty(clang::QualType* qt) {
    using namespace clang;
    using llvm::dyn_cast;

    auto ty = qt->getCanonicalType().getTypePtr();

    if(type_cache.contains(ty)) {
        return type_cache[ty];
    }

    Sem::Type result;

    if(const auto* bt = dyn_cast<BuiltinType>(ty)) {
        result = c_builtin_to_gear(bt);
    } else if(const auto* pt = dyn_cast<PointerType>(ty)) {
        QualType underlying = pt->getPointeeType();
        result = c_to_gear_ty(&underlying).ref();
    } else if(dyn_cast<RecordType>(ty)) {
        result = type_cache[ty];
    } else if(const auto* at = dyn_cast<ArrayType>(ty)) {
        QualType elem = at->getElementType();
        result = c_to_gear_ty(&elem).array();
    } else {
        throw std::runtime_error(ty->getTypeClassName());
    }

    type_cache[ty] = result;
    return result;
}

class FunctionVisitor : public clang::RecursiveASTVisitor<FunctionVisitor> {
public:
    bool VisitFunctionDecl(clang::FunctionDecl* func) {
        using clang::QualType;

        if (func->isThisDeclarationADefinition() || func->isExternC()) {
            using Ast::Nodes::Argument;

            std::deque<unique_ptr<Argument>> args;
            bool is_variadic = false;

            for (unsigned i = 0; i < func->getNumParams(); ++i) {
                QualType qt = func->getParamDecl(i)->getType();
                
                if (const auto* fn = qt->getAs<clang::FunctionProtoType>()) {
                    is_variadic = fn->isVariadic();
                    continue;
                }

                args.push_back(std::make_unique<Argument>(Argument(
                    "",
                    c_to_gear_ty(&qt),
                    { 0, 0, 0, 0 }
                )));
            }

            QualType ret = func->getReturnType();
            Ast::Nodes::ExternFn fn(
                func->getNameAsString(), 
                c_to_gear_ty(&ret),
                std::move(args),
                is_variadic,
                true,
                { 0, 0, 0, 0 }   
            );

            nodes.push_back(std::make_unique<Ast::Nodes::ExternFn>(std::move(fn)));

            std::cout << fn.to_string() << std::endl;
        }
        return true;
    }

    bool VisitRecordDecl(clang::RecordDecl* record) {
        if (record->isCompleteDefinition()) {
            c_record_to_gear(record);
        }

        return true;
    }

    bool VisitEnumDecl(clang::EnumDecl* enm) {
        if (enm->isComplete())
            std::cout << "Enum: " << enm->getNameAsString() << "\n";
        return true;
    }

    bool VisitTypedefNameDecl(clang::TypedefNameDecl* td) {
        std::cout << "Typedef: " << td->getNameAsString() << "\n";
        return true;
    }
};

class CFfiConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor visitor;
};

vector<unique_ptr<Ast::Nodes::NodeBase>> C_Ffi::compile_headers() {
    vector<unique_ptr<clang::ASTUnit>> asts;

    auto ast = clang::tooling::buildASTFromCodeWithArgs(
        src.str(), 
        C_FFI_ARGS,
        "ffiwrap.c"
    );

    if (ast) {
        CFfiConsumer consumer;
        consumer.HandleTranslationUnit(ast->getASTContext());
        asts.push_back(std::move(ast));
    } else {
        std::cerr << "Failed to parse C FFI: \n";
    }

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
    auto split = splitStringstream(filename, '/');
    if (split.size() == 1 || split[0] == "loc") {
        add_header_to_file(format("#include \"{}\"", split.back()));
    } else if (split[0] == "sys") {
        add_header_to_file(format("#include <{}>", split[1]));
    } else {
        assert(false && "Unexpected header path!");
    }
}
