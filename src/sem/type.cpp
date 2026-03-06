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

#include <gearlang/sem/type.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

using namespace Sem;

unordered_map<string, shared_ptr<Type::Struct>> Type::struct_list = { };

/* ============================
   LLVM lowering
   ============================ */
static unordered_map<string, llvm::StructType*> struct_type_list_name;
static unordered_map<Type::Struct*, llvm::StructType*> struct_type_list_sem;

llvm::Type* Type::struct_to_llvm(Type& obj, Context& ctx, string name) {
    // gather the types together and convert
    vector<llvm::Type*> tys;
    tys.reserve(obj.struct_type->size());
    
    for(auto& param : *obj.struct_type) {
        tys.push_back(param.second.to_llvm(ctx));
    }

    llvm::StructType* ty = llvm::StructType::create(tys, name);

    struct_type_list_name.insert({ name, ty });
    struct_type_list_sem.insert({ obj.struct_type.get(), ty });
    return ty;
}

llvm::Type* Type::get_llvm_struct(string name, Struct& obj) {
    auto it = struct_type_list_name.find(name);
    assert(it != struct_type_list_name.end());
    return it->second;
}

llvm::Type* Type::get_llvm_struct() const {
    auto it = struct_type_list_sem.find(struct_type.get());
    assert(it != struct_type_list_sem.end());
    return it->second;
}

llvm::Type* Type::primitive_to_llvm(PrimType ty, Context& ctx) {
    switch (ty) {
        case PrimType::Bool: return llvm::Type::getInt1Ty(ctx.llvmCtx);
        case PrimType::Char: // Same as i8
        case PrimType::I8: return llvm::Type::getInt8Ty(ctx.llvmCtx);
        case PrimType::I16: return llvm::Type::getInt16Ty(ctx.llvmCtx);
        case PrimType::I32:  return llvm::Type::getInt32Ty(ctx.llvmCtx);
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
        llvm::Type* base;

        if(pointer > 1) {
            // If pointers are stacked we don't truly care about what goes below the next
            base = llvm::PointerType::getUnqual(ctx.llvmCtx);
        } else {
            base = underlying_to_llvm(ctx);
        }

        return llvm::PointerType::get(base, 0);
    }

    throw std::runtime_error("Unknown non-primitive kind");
}

llvm::Type* Type::get_underlying_type(Context& ctx) const {
    if (!is_pointer_ty()) return nullptr;

    if(pointer > 1) {
        return llvm::PointerType::getUnqual(ctx.llvmCtx);
    }

    return primitive_to_llvm(prim_type, ctx);
}

/* ============================
   Queries
   ============================ */

bool Type::is_pointer_ty() const {
    return pointer;
}

int Type::pointer_level() const {
    if(!is_pointer_ty()) return -1;

    return pointer;
}

std::string Type::dump() {
    std::string prim;

    using enum PrimType;
    switch(prim_type) {
        case(Void): prim = "void"; break;
        case(Bool): prim = "bool"; break;
        case(Char): prim = "char"; break;
        case(I8): prim = "i8"; break;
        case(I16): prim = "i16"; break;
        case(I32): prim = "i32"; break;
        case(F32): prim = "f32"; break;
        case(F64): prim = "f64"; break;
        default: prim = "invalid"; break; 
    }

    if(is_primitive()) return prim;

    std::string s = prim;
    for(unsigned int i = 1; i < pointer; i++)
        s.push_back('^');

    return s;
}

bool Type::is_float() const {
    if(!is_primitive()) return false;

    using enum PrimType;
    switch(prim_type) {
        case(F32):
        case(F64): return true;
        default: return false;
    }
}

bool Type::is_int() const {
    if(!is_primitive()) return false;

    if(prim_type >= PrimType::Char && prim_type <= PrimType::I32) return true;
    return false;
}

bool Type::is_compatible(Type&& other) {
    if(is_pointer_ty() && other.is_pointer_ty()) return true;
    if(is_float() && other.is_float()) return true;
    if(is_int() && other.is_int()) return true;

    return false;
}

int Type::struct_parameter_index(string name) {
    if(!is_struct()) {
        throw std::logic_error("Expected a struct type");
    }

    for(unsigned int i = 0; i < struct_type->size(); i++) {
        pair<string, Type>& curr = struct_type->at(i);
        if(name == curr.first) {
            return i;
        }
    }

    return -1;
}
