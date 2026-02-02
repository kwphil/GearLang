#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

/// @brief Code generation context
class Context {
public:
    /// @brief LLVM components for code generation
    llvm::LLVMContext llvmCtx;
    /// @brief IR builder for generating LLVM instructions
    llvm::IRBuilder<> builder;
    /// @brief LLVM module representing the generated code
    std::unique_ptr<llvm::Module> module;

    /// @brief Start block for code outside functions
    std::unique_ptr<llvm::BasicBlock*> _start_block; 
    /// @brief Currently active function being generated
    llvm::Function* current_fn;

    Context()
    : builder(llvmCtx), module(std::make_unique<llvm::Module>("gearlang", llvmCtx)) {
        scopes.emplace_back(); // global scope
    }

    /// @brief Stack of variable scopes
    std::vector<std::unordered_map<std::string, llvm::Value*>> scopes;

    /// @brief Create an alloca instruction in the entry block of a function
    /// @param function Function in which to create the alloca
    /// @param name Name of the allocated variable
    /// @param type Type of the allocated variable
    /// @return Pointer to the created alloca instruction
    llvm::AllocaInst* create_entry_block(
        llvm::Function* function,
        const std::string& name,
        llvm::Type* type
    );

    /// @brief Render the current LLVM module to a string
    std::string render();

    /// @brief Push a new variable scope onto the stack
    void push_scope();
    /// @brief Pop the current variable scope from the stack
    void pop_scope();
    /// @brief Lookup a variable by name in the current scopes
    /// @param name Name of the variable to lookup
    /// @return Pointer to the LLVM value of the variable, or nullptr if not found
    llvm::Value* lookup(const std::string& name);
    /// @brief Bind a variable name to an LLVM value in the current scope
    /// @param name Name of the variable
    /// @param val Pointer to the LLVM value to bind
    void bind(const std::string& name, llvm::Value* val);
};
