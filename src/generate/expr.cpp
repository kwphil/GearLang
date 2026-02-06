#include <format>

#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

#include "../ast.hpp"
#include "../error.hpp"

// Generates both sides of the expression, and stores them in temporary values
// Matches through each operation and stores the output as a temp
// Returns the temporary variable
llvm::Value* Ast::Nodes::ExprOp::generate(Context& ctx) {
    llvm::Value* lhs = left->generate(ctx);
    llvm::Value* rhs = right->generate(ctx);

    if(lhs->getType()->isDoubleTy()) {
        switch(type) {
            case Add: return ctx.builder.CreateFAdd(lhs, rhs, "faddtmp");
            case Sub: return ctx.builder.CreateFSub(lhs, rhs, "fsubtmp");
            case Mul: return ctx.builder.CreateFMul(lhs, rhs, "fmultmp");
            case Div: return ctx.builder.CreateFDiv(lhs, rhs, "fdivtmp");
        }
    }

    switch (type) {
        case Add: return ctx.builder.CreateAdd(lhs, rhs, "iaddtmp");
        case Sub: return ctx.builder.CreateSub(lhs, rhs, "isubtmp");
        case Mul: return ctx.builder.CreateMul(lhs, rhs, "imultmp");
        case Div: return ctx.builder.CreateSDiv(lhs, rhs, "idivtmp");
    }

    int line = line_number; // It doesn't like using the line_number from the class
    
    Error::throw_error(
        line,
        "",
        "Invalid ExprOp",
        Error::ErrorCodes::INVALID_AST
    );
}

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise:
// Checks if it is a global and returns it
llvm::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    llvm::Value* var = ctx.lookup(name);
    if (!var)
        Error::throw_error(
            line_number,
            name.c_str(),
            "Unknown variable",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );

    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        return ctx.builder.CreateLoad(
            gv->getValueType(),
            gv,
            name + ".load"
        );
    }

    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(var);

    return ctx.builder.CreateLoad(
        alloca->getAllocatedType(),
        var,
        name + ".load"
    );
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
llvm::Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    llvm::Value* alloca = ctx.lookup(name);
    if (!alloca)
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to assign a variable that wasn't defined",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );

    llvm::Value* value = expr->generate(ctx);
    ctx.builder.CreateStore(value, alloca);

    return value;
}

// Gets the function by name, and creates a call for it
// Parses the expressions for each argument and calls it
llvm::Value* Ast::Nodes::ExprCall::generate(Context& ctx) {
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
        arg_values.push_back(a->generate(ctx));
    }

    return ctx.builder.CreateCall(func, arg_values);
}

#include <iostream>

llvm::Value* Ast::Nodes::ExprAddress::generate(Context& ctx) {
    llvm::Value* var = ctx.lookup(name);

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
    return var;
}

llvm::Value* deref(
    Context& ctx, llvm::Value* ptr, std::string& name, int line_number) {
    // First try if it is a global
    if(auto* gptr = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
        return ctx.builder.CreateLoad(
            gptr->getValueType(),
            gptr,
            name
        );
    }

    // Parse as an alloca otherwise
    llvm::AllocaInst* alloca_ptr;
    if(!(alloca_ptr = llvm::dyn_cast<llvm::AllocaInst>(var))) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to dereference a variable that wasn't declared",
            Error::ErrorCodes::VARIABLE_NOT_DEFINED
        );
    }

    return ctx.builder.CreateLoad(
        
    )
}

llvm::Value* Ast::Nodes::ExprDeref::generate(Context& ctx) {
    llvm::Value* var = ctx.lookup(name);

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

    // First globals
    if(auto* gptr = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        llvm::Value* ptr = ctx.builder.CreateLoad(
            gptr->getValueType(),
            gptr,
            var->getName() + ".load"
        );

        // Do it again

        if(gptr = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
            
        }
    }

    llvm::AllocaInst* alloca_ptr;
    if(!(alloca_ptr = llvm::dyn_cast<llvm::AllocaInst>(var))) {
        throw std::runtime_error("You shouldn't be here! deref");
    }

    llvm::Value* ptr = ctx.builder.CreateLoad(
        alloca_ptr->getAllocatedType(),
        var, var->getName() + ".load"
    );

    llvm::AllocaInst* alloca_val;
    if(!(alloca_val = llvm::dyn_cast<llvm::AllocaInst>(ptr))) {
        Error::throw_error(
            line_number,
            name.c_str(),
            "Tried to dereference a non-pointer",
            Error::ErrorCodes::BAD_TYPE
        );
    }
    
    return ctx.builder.CreateLoad(
        alloca_val->getAllocatedType(),
        ptr, var->getName() + ".deref"
    );
}