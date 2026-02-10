#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include "../ast/expr.hpp"
#include "../error.hpp"

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
Ast::Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    Ast::Value* lhs = left->generate(ctx);
    Ast::Value* rhs = right->generate(ctx);
    llvm::Value* out;

    if(lhs->ir->getType() != rhs->ir->getType()) {
        Error::throw_error(
            line_number,
            "",
            "Mismatched types",
            Error::ErrorCodes::BAD_TYPE
        );
    }

    if(lhs->ir->getType()->isDoubleTy()) {
        switch(type) {
            case Add: out = ctx.builder.CreateFAdd(lhs->ir, rhs->ir, "faddtmp");
            case Sub: out = ctx.builder.CreateFSub(lhs->ir, rhs->ir, "fsubtmp");
            case Mul: out = ctx.builder.CreateFMul(lhs->ir, rhs->ir, "fmultmp");
            case Div: out = ctx.builder.CreateFDiv(lhs->ir, rhs->ir, "fdivtmp");
        }
    } else {
        switch (type) {
            case Add: out = ctx.builder.CreateAdd(lhs->ir, rhs->ir, "iaddtmp");
            case Sub: out = ctx.builder.CreateSub(lhs->ir, rhs->ir, "isubtmp");
            case Mul: out = ctx.builder.CreateMul(lhs->ir, rhs->ir, "imultmp");
            case Div: out = ctx.builder.CreateSDiv(lhs->ir, rhs->ir, "idivtmp");
        }
    }

    if(out) {
        return new Ast::Value {
            .ir=out,
            .ty=lhs->ir->getType(),
            .is_address = lhs->is_address
        };
    }

    Error::throw_error(
        line_number,
        "",
        "Invalid ExprOp",
        Error::ErrorCodes::INVALID_AST
    );
}

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise:
// Checks if it is a global and returns it
Ast::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    Ast::Value* var = ctx.lookup(name);
    if (!var)
        Error::throw_error(
            line_number,
            name.c_str(),
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );

    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var->ir)) {
        llvm::Value* ir = ctx.builder.CreateLoad(
            gv->getValueType(),
            gv,
            name + ".load"
        );

        return new Value {
            .ir=ir,
            .ty=var->ty,
            .is_address=var->is_address
        };
    }

    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(var->ir);

    llvm::Value* ir = ctx.builder.CreateLoad(
        alloca->getAllocatedType(),
        var->ir,
        name + ".load"
    );

    return new Value {
        .ir=ir,
        .ty=var->ty,
        .is_address=var->is_address
    };
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
Ast::Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    Ast::Value* alloca = ctx.lookup(name);
    if (!alloca)
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to assign a variable that wasn't defined",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );

    Expr* expr2 = dynamic_cast<Expr*>(expr.get());
    
    if(!expr2) {
        Error::throw_error(
            line_number,
            "",
            "Expected rvalue",
            Error::ErrorCodes::INVALID_AST
        );
    }
    
    Ast::Value* value = expr2->generate(ctx);

    // type checking
    if(value->is_address != alloca->is_address
       || value->ty != value->ty
    ) {
        Error::throw_error(
            line_number,
            "=",
            "Mismatched types",
            Error::ErrorCodes::BAD_TYPE
        );
    }

    ctx.builder.CreateStore(value->ir, alloca->ir);

    return value;
}

// Gets the function by name, and creates a call for it
// Parses the expressions for each argument and calls it
Ast::Value* Ast::Nodes::ExprCall::generate(Context& ctx) {
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

    llvm::Value* val = ctx.builder.CreateCall(func, arg_values);

    return new Value {
        .ir=val,
        .ty=val->getType(),
        .is_address=false
    };
}

Ast::Value* Ast::Nodes::ExprAddress::generate(Context& ctx) {
    Ast::Value* var = ctx.lookup(name);

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
        var->ir,
        var->ty,
        true
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

Ast::Value* Ast::Nodes::ExprDeref::generate(Context& ctx) {
    Ast::Value* var = ctx.lookup(name);

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

    if(!var->is_address) {
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
        .is_address=false
    };
}
