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
#include <gearlang/error.hpp>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <format>
#include <unordered_set>

using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::unique_ptr;
using std::string;
using std::format;

using namespace Ast::Nodes;
using namespace clang;

const vector<string> C_FFI_ARGS = {
    "-I.",
    "-std=c99"
};

Sem::Type c_builtin_to_gear(const BuiltinType* ty) {
    using clang::BuiltinType;

    switch(ty->getKind()) {
        case BuiltinType::Int: return Sem::Type("i32");     
        case BuiltinType::Float: return Sem::Type("f32");   
        case BuiltinType::Double: return Sem::Type("f64");  
        case BuiltinType::Bool: return Sem::Type("bool");   
        case BuiltinType::Long: return Sem::Type("i64");   
        case BuiltinType::LongLong: return Sem::Type("i64");
        case BuiltinType::Char_S: return Sem::Type("i8");   
        case BuiltinType::Void: return Sem::Type("void");   
        case BuiltinType::ULong: return Sem::Type("u64");   
        case BuiltinType::UInt: return Sem::Type("u32");    
        case BuiltinType::UShort: return Sem::Type("u16");
        case BuiltinType::SChar: return Sem::Type("i8");

        default: 
            LangOptions LO;  // default language options
            PrintingPolicy policy(LO);
            throw std::runtime_error(ty->getName(policy).str()); 
    }
}

std::shared_ptr<Sem::Type> C_Ffi::c_record_to_gear(const RecordDecl* decl) {
    const clang::Type* canonical = decl->getTypeForDecl();
    if (type_cache.count(canonical))
        return type_cache[canonical];

    auto sem_ty = std::make_shared<Sem::Type>(
        Sem::Type::new_record(decl->getNameAsString(), decl->isUnion())
    );
    type_cache[canonical] = sem_ty; 
    
    for (auto* field : decl->fields()) {
        QualType field_ty = field->getType();
        sem_ty->record_add_param(
            field->getNameAsString(),
            c_to_gear_ty(&field_ty)
        );
    }

    return sem_ty;
}

std::shared_ptr<Sem::Type> C_Ffi::c_to_gear_ty(QualType* qt) {
    using namespace clang;
    using llvm::dyn_cast;

    auto ty = qt->getCanonicalType().getTypePtr();

    if (type_cache.contains(ty)) {
        return type_cache[ty];
    }

    std::shared_ptr<Sem::Type> result;

    if (auto* bt = dyn_cast<BuiltinType>(ty)) {
        result = std::make_shared<Sem::Type>(c_builtin_to_gear(bt));
    } 
    else if (auto* pt = dyn_cast<PointerType>(ty)) {
        QualType underlying = pt->getPointeeType();
        result = std::make_shared<Sem::Type>(c_to_gear_ty(&underlying)->ref());
    } 
    else if (auto* rt = dyn_cast<RecordType>(ty)) {
        result = c_record_to_gear(rt->getDecl());
    } 
    else if (auto* cat = dyn_cast<ConstantArrayType>(ty)) {
        QualType elem = cat->getElementType();
        uint64_t size = cat->getSize().getZExtValue();
        result = std::make_shared<Sem::Type>(c_to_gear_ty(&elem)->array(size));
    } 
    else if (auto* at = dyn_cast<ArrayType>(ty)) {
        QualType elem = at->getElementType();
        result = std::make_shared<Sem::Type>(c_to_gear_ty(&elem)->array());
    } 
    else {
        throw std::runtime_error(ty->getTypeClassName());
    }

    type_cache[ty] = result;
    return result;
}

#include <iostream>
class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> {
private:
    C_Ffi& manager;

public:
    FunctionVisitor(C_Ffi& manager) : manager(manager) { }

    bool VisitFunctionDecl(FunctionDecl* func) {
        using clang::QualType;

        if (func->isThisDeclarationADefinition() || func->isExternC()) {
            using Ast::Nodes::Argument;

            std::deque<unique_ptr<Argument>> args;
            bool is_variadic = func->isVariadic();

            for (unsigned i = 0; i < func->getNumParams(); ++i) {
                QualType qt = func->getParamDecl(i)->getType();

                args.push_back(std::make_unique<Argument>(Argument(
                    "",
                    *manager.c_to_gear_ty(&qt),
                    { 0, 0, 0, 0 }
                )));
            }

            QualType ret = func->getReturnType();
            ExternFn fn(
                func->getNameAsString(), 
                *manager.c_to_gear_ty(&ret),
                std::move(args),
                is_variadic,
                true,
                { 0, 0, 0, 0 }   
            );

            manager.add_node(std::make_unique<ExternFn>(std::move(fn)));
        }
        return true;
    }

    bool VisitRecordDecl(RecordDecl* record) {
        if (record->isCompleteDefinition()) {
            manager.c_record_to_gear(record);
        }

        return true;
    }

    bool VisitEnumDecl(EnumDecl* enm) {
        // if (enm->isComplete())
        //     std::cout << "Enum: " << enm->getNameAsString() << "\n";
        return true;
    }

    bool VisitTypedefNameDecl(TypedefNameDecl* td) {
        // std::cout << "Typedef: " << td->getNameAsString() << "\n";
        return true;
    }
};

class CFfiConsumer : public ASTConsumer {
public:
    CFfiConsumer(C_Ffi& manager) : visitor(manager) { }

    void HandleTranslationUnit(ASTContext& context) override {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor visitor;
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
