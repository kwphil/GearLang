#include "ast.hpp"

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>

#include "syscall.hpp"
#include "var.hpp"

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

    throw std::runtime_error("Invalid ExprOp");
}

// Just generates an int constant and returns it
// TODO: Support different bit widths for optimization 
llvm::Value* Ast::Nodes::ExprLitInt::generate(Context& ctx) {
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(ctx.llvmCtx),
        this->value,
        true
    );
}

// TODO
llvm::Value* Ast::Nodes::ExprLitFloat::generate(Context& ctx) {
    return llvm::ConstantFP::get(
        // TODO: Support different float types for optimization
        llvm::Type::getDoubleTy(ctx.llvmCtx),
        this->value
    );
};

// Converts a string into an array of constant i8s
// Creates a constant array using that
// Returns a pointer to the string
llvm::Value* Ast::Nodes::ExprLitString::generate(Context& ctx) {
    std::vector<llvm::Constant*> chars(string.size());
    llvm::Type* i8 = llvm::Type::getInt8Ty(ctx.llvmCtx);
    llvm::ArrayType* arr_i8 = llvm::ArrayType::get(i8, chars.size());

    for(int i = 0; i < string.size(); i++) {
        chars[i] = llvm::ConstantInt::get(i8, string[i]);
    }

    auto array = llvm::ConstantArray::get(
        arr_i8, 
        chars
    );

    return new llvm::GlobalVariable(
        *ctx.module,
        arr_i8,
        true,
        llvm::GlobalValue::ExternalLinkage,
        array,
        ".str"
    );
}

// Looks up the name of the variable
// If the variable doesn't exist, it throws an error and quits
// Otherwise:
// Checks if it is a global and returns it
llvm::Value* Ast::Nodes::ExprVar::generate(Context& ctx) {
    llvm::Value* var = ctx.lookup(name);
    if (!var)
        throw std::runtime_error("Unknown variable: " + name);

    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        return ctx.builder.CreateLoad(
            gv->getValueType(),
            gv,
            name + ".load"
        );
    }

    return ctx.builder.CreateLoad(
        var->getType(),
        var,
        name + ".load"
    );
}

// Assigns a value to a variable, and stores the value in the variable's alloca
// If the variable doesn't exist, it throws an error and quits
llvm::Value* Ast::Nodes::ExprAssign::generate(Context& ctx) {
    llvm::Value* alloca = ctx.lookup(name);
    if (!alloca)
        throw std::runtime_error("Unknown variable: " + name);

    llvm::Value* value = expr->generate(ctx);
    ctx.builder.CreateStore(value, alloca);

    return value;
}

// Checks if the variable already exists
// If it does, it throws an error and quits
// Otherwise, it creates the variable in the current scope
// If in the global scope, it creates a global variable
llvm::Value* Ast::Nodes::Let::generate(Context& ctx) {
    llvm::Value* initVal = expr->generate(ctx);

    // Making sure that the variable doesn't exist
    auto _var = ctx.lookup(target);
    
    if(_var) {
        throw std::runtime_error("Variable already defined: " + target);
    }

    // GLOBAL SCOPE
    llvm::Function* _fn = *ctx.current_fn;
    if (_fn->getName() == "_start") {
        llvm::Type* ty = initVal->getType();

        // Creating a placeholder and then assigning the value
        llvm::Constant* placeholder = 
            llvm::Constant::getNullValue(ty);
        
        // Create the variable
        llvm::GlobalVariable* var = new llvm::GlobalVariable(
            *(ctx.module),
            ty,
            false,
            llvm::GlobalValue::ExternalLinkage,
            placeholder,
            target
        );

        ctx.bind(target, var);

        ctx.builder.CreateStore(initVal, var);

        return var;
    }

    // FUNCTION SCOPE
    llvm::Function* fn = *ctx.current_fn;
    llvm::AllocaInst* alloca =
        ctx.create_entry_block(fn, target, initVal->getType());

    ctx.builder.CreateStore(initVal, alloca);
    ctx.bind(target, alloca);
    return alloca;
}

// Creates a new scope, generates all the expressions inside the block,
// then pops the scope
llvm::Value* Ast::Nodes::ExprBlock::generate(Context& ctx) {
    ctx.push_scope();
    for (auto& expr : nodes)
        expr->generate(ctx);
    ctx.pop_scope();
    return nullptr;
}

// Generates the return value, ensures it is i32
// Then creates a syscall to exit with that return value
llvm::Value* Ast::Nodes::Return::generate(Context& ctx) {
    llvm::Value* retVal = expr->generate(ctx);

    if(!ctx.module->getFunction("main") && !ctx.module->getFunction("_start")) {
        throw std::runtime_error("Unimplemented return function for non-main function");
    }

    // Ensure i32
    if (retVal->getType()->isIntegerTy() &&
        retVal->getType()->getIntegerBitWidth() < 32) {
        retVal = ctx.builder.CreateSExt(
            retVal,
            llvm::Type::getInt32Ty(ctx.llvmCtx)
        );
    }

    auto asmFn = syscall_exit(ctx.llvmCtx);

    ctx.builder.CreateCall(asmFn, { retVal });
    ctx.builder.CreateUnreachable();
    return retVal;
}

// Generates the if condition, creates blocks for the if and the continuation
// Generates the 'then' block inside the if block
// Jumps to the continuation block afterwards   
llvm::Value* Ast::Nodes::If::generate(Context& ctx) {
    llvm::Function* fn = ctx.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* if_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.true", fn);
    llvm::BasicBlock* then_block =
        llvm::BasicBlock::Create(ctx.llvmCtx, "if.end", fn);

    // Generate condition ONCE
    llvm::Value* condVal = cond->generate(ctx);

    // Convert to boolean: cond != 0
    // TODO: Support different types (String would be x != "", etc)
    llvm::Value* condv = ctx.builder.CreateICmpNE(
        condVal,
        llvm::ConstantInt::get(
            condVal->getType(),
            0,
            true
        ),
        "ifcond"
    );

    ctx.builder.CreateCondBr(condv, if_block, then_block);

    // if (cond)
    ctx.builder.SetInsertPoint(if_block);
    expr->generate(ctx);
    ctx.builder.CreateBr(then_block);

    // continuation
    ctx.builder.SetInsertPoint(then_block);

    return nullptr;
}

// Creates the function type. 
// Creates the function and adds it to the module
// Creates the entry basic block and sets the insert point
// Generates the function body
// Pops the scope and resets the current function
// Sets the insert point back to the start block
llvm::Value* Ast::Nodes::Function::generate(Context& ctx) {
    std::vector<llvm::Type*> param_types;
    param_types.reserve(args.size());
    for (auto& arg : args) {
        param_types.push_back(
            Ast::type_to_llvm_type(arg.ty, ctx)
        );
    }

    /* 2. Create function type */
    llvm::FunctionType* fn_type =
        llvm::FunctionType::get(
            Ast::type_to_llvm_type(ty, ctx),
            param_types,
            false
        );

    llvm::Function* fn =
        llvm::Function::Create(
            fn_type,
            llvm::Function::ExternalLinkage,
            name,
            *ctx.module
        );

    unsigned idx = 0;
    for (auto& llvm_arg : fn->args()) {
        llvm_arg.setName(args[idx++].name);
    }

    ctx.current_fn = std::make_shared<llvm::Function*>(fn);
    ctx.push_scope();

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(ctx.llvmCtx, "entry", fn);
    ctx.builder.SetInsertPoint(entry);

    idx = 0;
    for (auto& llvm_arg : fn->args()) {
        auto& ast_arg = args[idx];

        llvm::AllocaInst* alloca =
            ctx.create_entry_block(
                fn,
                ast_arg.name,
                Ast::type_to_llvm_type(ast_arg.ty, ctx)
            );

        ctx.builder.CreateStore(&llvm_arg, alloca);
        ctx.bind(ast_arg.name, alloca);

        idx++;
    }

    block->generate(ctx);

    if (!entry->getTerminator()) {
        if (ty == Ast::Type::Void) {
            ctx.builder.CreateRetVoid();
        } else {
            ctx.builder.CreateRet(
                llvm::Constant::getNullValue(
                    Ast::type_to_llvm_type(ty, ctx)
                )
            );
        }
    }

    ctx.pop_scope();
    ctx.current_fn.reset();
    ctx.builder.SetInsertPoint(*ctx._start_block);

    return fn;
}

void Ast::Program::generate(Context& ctx) {
    for (const auto& expr : content)
        expr->generate(ctx);
}
