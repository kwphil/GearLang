#include <llvm/IR/Type.h>

#include "var.hpp"
#include "ctx.hpp"
#include "lex.hpp"
#include "error.hpp"

#include <iostream>

Ast::Type::PrimType Ast::Type::parse_type(std::string& s) {
    if(s == "void") return PrimType::Void;
    if(s == "char") return PrimType::Char;
    if(s == "i32") return PrimType::I32;
    if(s == "f32") return PrimType::F32;
    if(s == "f64") return PrimType::F64;

    // Base case
    return PrimType::Invalid;
}

Ast::Type::PrimType Ast::Type::parse_type(Lexer::Stream& s) {
    if(s.next()->type == Lexer::Type::Amper) { 
        return PrimType::NonPrimitive; 
    }

    return parse_type(s.pop()->content);
}

Ast::Type::NonPrimitive* Ast::Type::parse_nonprim(Lexer::Stream& s) {    
    const auto& curr = s.pop();
    auto ty = curr->content;
    
    if(s.peek()->type == Lexer::Type::Amper) {
        s.pop();

        return new NonPrimitive({ // Pointer
            0,
            (int)parse_type(ty)
        });
    }

    throw std::runtime_error("Unknown NonPrimitive Type: " + curr->content);
}

llvm::Type* Ast::Type::type_to_llvm_type_np(Context& ctx) {
    // [0, x] where x is the primitive it's referring to.
    // nonprimitive pointers are a future me problem
    // so expect it to crash for nonprimitives
    if(non_prim->type[0] == 0) { // Pointer
        return llvm::PointerType::get(type_to_llvm_type((PrimType)non_prim->type[1], ctx), 0);
    }

    throw std::runtime_error("Unknown primitive type: " + non_prim->type[0]);
}

llvm::Type* Ast::Type::type_to_llvm_type(Context& ctx) {    
    return type_to_llvm_type(prim_type, ctx);
} 

llvm::Type* Ast::Type::type_to_llvm_type(PrimType ty, Context& ctx) {
    switch(ty) {
        case(PrimType::Char): return llvm::Type::getInt8Ty(ctx.llvmCtx);
        case(PrimType::I32):  return llvm::Type::getInt32Ty(ctx.llvmCtx);
        case(PrimType::F32):  return llvm::Type::getFloatTy(ctx.llvmCtx);
        case(PrimType::F64):  return llvm::Type::getDoubleTy(ctx.llvmCtx);
        case(PrimType::Void): return llvm::Type::getVoidTy(ctx.llvmCtx);
        // Safety check
        case(PrimType::NonPrimitive): throw std::runtime_error("Tried to parse a nonprim as a prim!");
        default: throw std::runtime_error("You shouldn't be here!!!!!! " + (int)ty);
    }
}

Ast::Type::Type(Lexer::Stream& s) {
    std::unique_ptr<Lexer::Token> t = s.peek();

    prim_type = parse_type(s);

    if(prim_type == PrimType::NonPrimitive) {
        non_prim = parse_nonprim(s);
    }

    if(prim_type == PrimType::Invalid) {
        Error::throw_error(
            t->line,
            t->content.c_str(),
            "Unknown type",
            Error::ErrorCodes::UNKNOWN_TYPE
        );
    }
}

llvm::Type* Ast::Type::generate(Context& ctx) {
    return prim_type == PrimType::NonPrimitive
    ? type_to_llvm_type_np(ctx)
    : type_to_llvm_type(ctx);
}