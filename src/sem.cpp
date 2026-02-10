#include "sem.hpp"
#include "ctx.hpp"
#include "lex.hpp"
#include "error.hpp"

#include <format>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

using namespace Sem;

/* ============================
   Primitive parsing
   ============================ */

Type::PrimType Type::parse_primitive(std::string& s) {
    if (s == "void") return PrimType::Void;
    if (s == "char") return PrimType::Char;
    if (s == "i32")  return PrimType::I32;
    if (s == "f32")  return PrimType::F32;
    if (s == "f64")  return PrimType::F64;
    return PrimType::Invalid;
}

Type::PrimType Type::parse_primitive(Lexer::Stream& s) {
    return parse_primitive(s.pop()->content);
}

/* ============================
   Non-primitive parsing
   ============================ */

Type::NonPrimitive Type::parse_nonprimitive(Lexer::Stream& s, PrimType prim_type) {
    // Currently only pointer types are supported: T&
    // Encoding: [0, <PrimType>]
    // 0 = pointer tag


    if (s.peek()->type == Lexer::Type::Amper) {
        s.pop(); // consume '&'
        return NonPrimitive{
            { 0, static_cast<int>(prim_type) }
        };
    }

    throw std::runtime_error(
        std::format("Unknown non-primitive type: {}", (int)prim_type));
}

/* ============================
   Construction
   ============================ */

#include <iostream>

Type::Type(Lexer::Stream& s) {
    auto tok = s.peek();

    prim_type = parse_primitive(s);

    // Pointer type
    if (s.peek()->type == Lexer::Type::Amper) {
        non_prim = parse_nonprimitive(s, prim_type);
        return;
    }

    if (prim_type == PrimType::Invalid) {
        Error::throw_error(
            tok->line,
            tok->content.c_str(),
            "Unknown type",
            Error::ErrorCodes::UNKNOWN_TYPE
        );
    }
}

/* ============================
   LLVM lowering
   ============================ */

llvm::Type* Type::primitive_to_llvm(PrimType ty, Context& ctx) {
    switch (ty) {
        case PrimType::Char: return llvm::Type::getInt8Ty(ctx.llvmCtx);
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

    // Non-primitive lowering
    const auto& np = non_prim.value();

    // Pointer
    if (np.type[0] == 0) {
        auto base =
            primitive_to_llvm(
                static_cast<PrimType>(np.type[1]),
                ctx
            );

        return llvm::PointerType::get(base, 0);
    }

    throw std::runtime_error("Unknown non-primitive kind");
}

llvm::Type* Type::get_underlying_type(Context& ctx) const {
    if (!is_pointer_ty()) return nullptr;

    return primitive_to_llvm(
        static_cast<PrimType>(non_prim->type[1]),
        ctx
    );
}

/* ============================
   Queries
   ============================ */

bool Type::is_pointer_ty() const {
    return non_prim.has_value() && non_prim->type[0] == 0;
}
