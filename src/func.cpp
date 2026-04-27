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

#include <gearlang/func.hpp>
#include <gearlang/ctx.hpp>

using std::string;

llvm::Function* declare_func(
    llvm::Type* ret_type,
    llvm::ArrayRef<llvm::Type*> args,
    const char* name, Context& ctx,
    bool variadic,
    bool is_public 
) {
    llvm::FunctionType* fn_type = llvm::FunctionType::get(
        ret_type,
        args,
        variadic
    );

    return llvm::Function::Create(
        fn_type,
        is_public 
        ? llvm::GlobalValue::ExternalLinkage
        : llvm::GlobalValue::PrivateLinkage,
        name, *ctx.module
    );
}

void call_exit(Context& ctx, llvm::Value* retVal) {
    static llvm::Function* exit_fn;

    if(!exit_fn) { // If exit hasn't been created yet
        exit_fn = declare_func( // Setting up exit call
            llvm::Type::getVoidTy(ctx.llvmCtx),
            { llvm::Type::getInt32Ty(ctx.llvmCtx) },
            "exit", ctx, false
        );
    }

    ctx.builder.CreateCall(exit_fn, { retVal });
    ctx.builder.CreateUnreachable();
}

inline const char* itanium_prim_parse(Sem::Type& ty) {
    using enum Sem::Type::PrimType;

    switch(ty.prim_type) {
        case(Void): return "v";
        case(Char): return "c";
        case(Bool): return "b";
        case(I8):
        case(U8):   return "h";
        case(I16):  return "s";
        case(U16):  return "t";
        case(I32):  return "i";
        case(U32):  return "j";
        case(I64):  return "l";
        case(U64):  return "m";
        case(F32):  return "f";
        case(F64):  return "d";
        default: throw std::runtime_error(ty.dump());
    }
}

string mangle_identifier(Sem::Func handle) {
    string identifier;
    if(handle.mangle == ManglingScheme::None) {
        identifier = handle.name;
    } else if(handle.mangle == ManglingScheme::Itanium) {
        identifier = "_Z";

        vector<string> name_split = split_string(handle.name, '.');

        // Indicate N marker for nested identifiers
        if(name_split.size() > 1) {
            identifier += 'N';
        }

        for(auto& n : name_split) {
            identifier += std::to_string(n.length()) + n;
        }

        identifier += 'E';

        if(handle.args.size() == 0) {
            identifier += "v";
            return identifier;
        }

        for(auto& arg : handle.args) {
            if(arg.is_primitive()) {
                identifier += itanium_prim_parse(arg);
            }

            if(arg.is_pointer_ty()) {
                identifier += 
                    std::string(arg.pointer_level(), 'P') + 
                    itanium_prim_parse(arg);
            }
        }
    } else if(handle.mangle == ManglingScheme::MSVC) {
        throw std::runtime_error("unimplemented MSVC");
    } else if(handle.mangle == ManglingScheme::Gearlang) {
        identifier = "_g" + std::to_string(handle.name.length()) + handle.name + std::to_string(handle.args.size());
        for(auto& arg : handle.args) {
            identifier += '.';
            string dump = arg.dump();
            identifier += std::to_string(dump.length()) + dump;
        }
    } else { throw std::runtime_error("unknown mangle type"); }

    return identifier;
}