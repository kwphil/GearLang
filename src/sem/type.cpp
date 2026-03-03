
#include <gearlang/sem/type.hpp>
#include <gearlang/ctx.hpp>
#include <gearlang/lex.hpp>
#include <gearlang/error.hpp>

#include <format>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

using namespace Sem;
using std::optional;

/* ============================
   Primitive parsing
   ============================ */

Type::PrimType Type::parse_primitive(std::string& s) {
    if (s == "void") return PrimType::Void;
    if (s == "bool") return PrimType::Bool;
    if (s == "char") return PrimType::Char;
    if (s == "i8")   return PrimType::I8;
    if (s == "i16")  return PrimType::I16;
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
    // Encoding: [0, <count>, <PrimType>]
    // 0 = pointer tag

    if (s.peek()->type == Lexer::Type::Caret) {
        // How many pointers?
        int count = 0;
        while(s.has()) {
            if(s.peek()->type != Lexer::Type::Caret) break;

            s.pop();
            count++;
        }

        NonPrimitive ret;
        ret.reserve(3);

        ret = { 0, count, static_cast<int>(prim_type) };
        return ret;
    }

    throw std::runtime_error(
        std::format("Unknown non-primitive type: {}", (int)prim_type));
}

/* ============================
   Construction
   ============================ */

Type::Type(Lexer::Stream& s) {
    auto tok = s.peek();

    prim_type = parse_primitive(s);

    // Pointer type
    if (s.peek()->type == Lexer::Type::Caret) {
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

Type Type::ref() {
    return Type(std::format("{}^", dump()).c_str());
}

Type Type::deref() {
    NonPrimitive new_np = non_prim.value();

    assert(new_np.at(0) == 0);
    new_np.at(1)-=1;

    return Type(prim_type, new_np);
}

/* ============================
   LLVM lowering
   ============================ */

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

    // Non-primitive lowering
    const auto& np = non_prim.value();

    // Pointer
    if (np[0] == 0) {
        llvm::Type* base;

        if(np[1] > 1) {
            // If pointers are stacked we don't truly care about what goes below the next
            base = llvm::PointerType::getUnqual(ctx.llvmCtx);
        } else {
            base = primitive_to_llvm(
                static_cast<PrimType>(np[2]),
                ctx
            );
        }

        return llvm::PointerType::get(base, 0);
    }

    throw std::runtime_error("Unknown non-primitive kind");
}

llvm::Type* Type::get_underlying_type(Context& ctx) const {
    if (!is_pointer_ty()) return nullptr;

    if(non_prim->at(1) > 1) {
        return llvm::PointerType::getUnqual(ctx.llvmCtx);
    }

    return primitive_to_llvm(
        static_cast<PrimType>(non_prim->at(2)),
        ctx
    );
}

/* ============================
   Queries
   ============================ */

bool Type::is_pointer_ty() const {
    return non_prim.has_value() && non_prim->at(0) == 0;
}

int Type::pointer_level() const {
    if(!is_pointer_ty()) return -1;
    if(non_prim->at(0)) return -2;

    return non_prim->at(1);
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

    std::string s = "";
    for(int i = 0; i < non_prim.value().at(1); i++)
        s.push_back('&');

    s.append(prim);
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