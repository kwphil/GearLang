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
#include <clang/AST/Type.h>
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

#include "visitor.hpp"

using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::unique_ptr;
using std::string;
using std::format;

using namespace Ast::Nodes;
using namespace clang;

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
        case BuiltinType::UChar: return Sem::Type("u8");
        case BuiltinType::Short: return Sem::Type("i16");
        case BuiltinType::LongDouble: return Sem::Type("f64");

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


