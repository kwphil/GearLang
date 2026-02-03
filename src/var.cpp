#include <llvm/IR/Type.h>

#include "var.hpp"
#include "ctx.hpp"
#include "lex.hpp"
#include "error.hpp"

Ast::Type Ast::parse_type(std::string& s) {
    if(s == "void") return Type::Void;
    if(s == "char") return Type::Char;
    if(s == "i32") return Type::I32;
    if(s == "f32") return Type::F32;
    if(s == "f64") return Type::F64;

    // Base case
    return Type::Invalid;
}

Ast::Type Ast::parse_type(Lexer::Stream& s) {
    if(s.next()->type == Lexer::Type::Amper) { 
        return Type::NonPrimitive; 
    }

    return parse_type(s.pop()->content);
}

Ast::NonPrimitive Ast::parse_nonprim(Lexer::Stream& s) {    
    const auto& curr = s.pop();
    auto ty = curr->content;
    
    if(s.peek()->type == Lexer::Type::Amper) { 
        s.pop(); 
        return NonPrimitive{ .type= { // Pointer
            0,
            (int)parse_type(ty)
        }};
    }

    throw std::runtime_error("Unknown NonPrimitive Type: " + curr->content);
}

llvm::Type* Ast::type_to_llvm_type(Type ty, Context& ctx) {    
    switch(ty) {
        case(Type::Char): return llvm::Type::getInt8Ty(ctx.llvmCtx);
        case(Type::I32):  return llvm::Type::getInt32Ty(ctx.llvmCtx);
        case(Type::F32):  return llvm::Type::getFloatTy(ctx.llvmCtx);
        case(Type::F64):  return llvm::Type::getDoubleTy(ctx.llvmCtx);
        case(Type::Void): return llvm::Type::getVoidTy(ctx.llvmCtx);
        case(Type::NonPrimitive): throw std::runtime_error("Tried to parse a nonprimitive as a primitive");
        case(Type::Invalid): return nullptr;
        default: throw std::runtime_error("You shouldn't be here!!!!!! " + (int)ty);
    }
} 

llvm::Type* Ast::type_to_llvm_type(NonPrimitive ty, Context& ctx) {
    // [0, x] where x is the primitive it's referring to.
    // nonprimitive pointers are a future me problem
    // so expect it to crash for nonprimitives
    if(ty.type[0] == 0) { // Pointer
        return llvm::PointerType::get(type_to_llvm_type((Type)ty.type[1], ctx), 0);
    }

    throw std::runtime_error("Unknown primitive type: " + ty.type[0]);
}
