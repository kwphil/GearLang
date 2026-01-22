#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

class Context {
public:
    llvm::LLVMContext llvmCtx;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::unique_ptr<llvm::BasicBlock*> _start_block; 
    // Making sure that this is separate for code outside the functions

    std::shared_ptr<llvm::Function*> current_fn;

    Context()
    : builder(llvmCtx), module(std::make_unique<llvm::Module>("gearlang", llvmCtx)) {
        scopes.emplace_back(); // global scope
    }

    std::vector<std::unordered_map<std::string, llvm::Value*>> scopes;

    llvm::AllocaInst* create_entry_block(
        llvm::Function* function,
        const std::string& name,
        llvm::Type* type
    );
    
    void emit(std::string line);

    std::string render();

    void push_scope();
    void pop_scope();
    llvm::Value* lookup(const std::string& name);
    void bind(const std::string& name, llvm::Value* val);
};
