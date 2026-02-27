#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include <gearlang/ast/expr.hpp>
#include <gearlang/ast/stmt.hpp>
#include <gearlang/ast/func.hpp>

#include <gearlang/error.hpp>
#include <gearlang/etc.hpp>

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    Value* lhs = left->generate(ctx);
    Value* rhs = right->generate(ctx);
    llvm::Value* out;

    if(ty->is_float()) {
        switch(type) {
            case Add: out = ctx.builder.CreateFAdd(lhs->ir, rhs->ir, "faddtmp"); break;
            case Sub: out = ctx.builder.CreateFSub(lhs->ir, rhs->ir, "fsubtmp"); break;
            case Mul: out = ctx.builder.CreateFMul(lhs->ir, rhs->ir, "fmultmp"); break;
            case Div: out = ctx.builder.CreateFDiv(lhs->ir, rhs->ir, "fdivtmp"); break;
        }
    } else {
        switch (type) {
            case Add: out = ctx.builder.CreateAdd(lhs->ir, rhs->ir, "iaddtmp"); break;
            case Sub: out = ctx.builder.CreateSub(lhs->ir, rhs->ir, "isubtmp"); break;
            case Mul: out = ctx.builder.CreateMul(lhs->ir, rhs->ir, "imultmp"); break;
            case Div: out = ctx.builder.CreateSDiv(lhs->ir, rhs->ir, "idivtmp"); break;
        }
    }

    if(out) {
        return new Value {
            .ir=out,
            .ty=ty->to_llvm(ctx),
            .addr = lhs->addr
        };
    }

    Error::throw_error(
        line_number,
        "",
        "Invalid ExprOp",
        Error::ErrorCodes::INVALID_AST
    );
}

#include <iostream>

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise Checks if it is a global and returns it
Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    // Link to the declaration statement
    llvm::Value* var;

    // std::cout << name << ": " << let << std::endl;
    // Checking if it came from a let stmt
    if(Let* let_val = try_cast<NodeBase, Let>(let))
        var = let_val->var;
    else
        var = try_cast<NodeBase, Argument>(let)->var;


    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        llvm::Value* ir = ctx.builder.CreateLoad(
            ty->to_llvm(ctx),
            gv,
            name + ".load"
        );

        return new Value {
            .ir=ir,
            .ty=ty->to_llvm(ctx),
            .addr=ty->pointer_level()
        };
    }

    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(var);

    llvm::Value* ir = ctx.builder.CreateLoad(
        alloca->getAllocatedType(),
        var,
        name + ".load"
    );

    return new Value {
        .ir=ir,
        .ty=ty->to_llvm(ctx),
        .addr=ty->pointer_level()
    };
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    // Link to the declaration statement
    llvm::Value* var;

    // Checking if it came from a let stmt
    if(Let* let_val = try_cast<NodeBase, Let>(let))
        var = let_val->var;
    else
        var = try_cast<NodeBase, Argument>(let)->var;

    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    
    if(!expr2) {
        Error::throw_error(
            line_number,
            "",
            "Expected rvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    
    Value* value = expr2->generate(ctx);

    ctx.builder.CreateStore(value->ir, var);

    return value;
}

// Gets the function by name, and creates a call for it
// Parses the expressions for each argument and calls it
Value* Ast::Nodes::ExprCall::generate(Context& ctx) {
    llvm::Function* func = ctx.module->getFunction(callee);

    if(!func) {
        Error::throw_error(
            line_number,
            callee.c_str(),
            "Unknown function",
            Error::ErrorCodes::FUNCTION_NOT_DEFINED
        );
    }

    std::vector<llvm::Value*> arg_values;
    
    for(auto& a : args) {
        arg_values.push_back(a->generate(ctx)->ir);
    }

    if(func->getReturnType() == llvm::Type::getVoidTy(ctx.llvmCtx)) { 
        ctx.builder.CreateCall(func, arg_values); 
        return nullptr;
    }

    llvm::Value* val = ctx.builder.CreateCall(func, arg_values, "call");

    return new Value {
        .ir=val,
        .ty=val->getType(),
        .addr=0
    };
}

Value* Ast::Nodes::ExprAddress::generate(Context& ctx) {
    Value* var = ctx.lookup(name);

    if(!var) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to reference a variable that wasn't declared",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    // Just return the value because everything is allocated
    // TODO: Later I might optimize this to locate which variables
    // have to alloca and optimize ones that don't out
    return new Value {
        .ir=var->ir,
        .ty=var->ty,
        .addr=var->addr+1
    };
}

llvm::Value* deref(
    Context& ctx, 
    llvm::Value* ptr, 
    llvm::Type* type,
    std::string&& name,
    const char* suffix, 
    int line_number
) {
    // First try if it is a global
    if(auto* gptr = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
        return ctx.builder.CreateLoad(
            type,
            gptr,
            name + suffix
        );
    }

    return ctx.builder.CreateLoad(
        type,
        ptr,
        name + suffix
    );
}

Value* Ast::Nodes::ExprDeref::generate(Context& ctx) {
    Value* var = ctx.lookup(name);

    if(!var) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to dereference a variable that wasn't declared",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    // Load the variable twice and return
    // Once to get the pointer
    // Twice to get the dereferenced data

    if(!var->addr) {
        Error::throw_error(
            line_number,
            var->ir->getName().str().c_str(),
            "Expected a pointer type",
            Error::ErrorCodes::BAD_TYPE
        );
    }

    llvm::Value* load = deref(
        ctx,
        var->ir,
        var->ir->getType(),
        var->ir->getName().str(),
        ".load",
        line_number
    );

    llvm::Value* deref_var = deref(
        ctx,
        load,
        var->ty,
        var->ir->getName().str(),
        ".deref",
        line_number
    );

    return new Value {
        .ir=deref_var,
        .ty=var->ty,
        .addr=var->addr-1
    };
}
