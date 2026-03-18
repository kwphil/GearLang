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

#include <unordered_map>
#include <string>

#include <llvm/IR/Type.h>

#include <gearlang/sem/type.hpp>

using std::unordered_map;
using std::string;
using namespace Sem;

static unordered_map<string, llvm::StructType*> record_type_list;

llvm::Type* Type::struct_to_llvm(Type& obj, Context& ctx, string name) {
    // gather the types together and convert
    vector<llvm::Type*> tys;
    tys.reserve(obj.record_type->size());
    
    for(auto& param : *obj.record_type) {
        tys.push_back(param.second->to_llvm(ctx));
    }

    llvm::StructType* ty = llvm::StructType::create(tys, name);

    record_type_list.insert({ name, ty });
    return ty;
}

llvm::Type* Type::get_llvm_struct(string name, Struct& obj) {
    auto it = record_type_list.find(name);
    assert(it != record_type_list.end());
    return it->second;
}

llvm::Type* Type::get_llvm_struct() const {
    auto it = record_type_list.find(record_name);
    assert(it != record_type_list.end());
    return it->second;
}

llvm::Type* Type::primitive_to_llvm(PrimType ty, Context& ctx) {
    switch (ty) {
        case PrimType::Bool: return llvm::Type::getInt1Ty(ctx.llvmCtx);
        case PrimType::Char: // Same as i8
        case PrimType::U8:   // same as i8
        case PrimType::I8: return llvm::Type::getInt8Ty(ctx.llvmCtx);
        case PrimType::U16:  // Same as i16
        case PrimType::I16: return llvm::Type::getInt16Ty(ctx.llvmCtx);
        case PrimType::U32:  // Same as i32
        case PrimType::I32:  return llvm::Type::getInt32Ty(ctx.llvmCtx);
        case PrimType::U64:  // Same as i64
        case PrimType::I64:  return llvm::Type::getInt64Ty(ctx.llvmCtx);
        case PrimType::F32:  return llvm::Type::getFloatTy(ctx.llvmCtx);
        case PrimType::F64:  return llvm::Type::getDoubleTy(ctx.llvmCtx);
        case PrimType::Void: return llvm::Type::getVoidTy(ctx.llvmCtx);
        default:
            throw std::runtime_error("Invalid primitive type");
    }
}

llvm::Type* Type::to_llvm(Context& ctx) const {
    if (is_primitive()) {
        return primitive_to_llvm(prim_type, ctx);
    }

    if (!is_pointer_ty() && is_struct()) 
        return get_llvm_struct();

    // Pointer
    if (pointer) {
        return llvm::PointerType::get(ctx.llvmCtx, 0);
    }

    throw std::runtime_error("Unknown non-primitive kind");
}

llvm::Type* Type::get_underlying_type(Context& ctx) const {
    if (!is_pointer_ty()) {
        if(is_array()) return array_type->to_llvm(ctx);
    };

    if(pointer > 1) {
        return llvm::PointerType::getUnqual(ctx.llvmCtx);
    }

    return primitive_to_llvm(prim_type, ctx);
}
