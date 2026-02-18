#include <string>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>

#include "ctx.hpp"

llvm::AllocaInst* Context::create_entry_block(
    llvm::Function* function,
    const std::string& name,
    llvm::Type* type
) {
    llvm::IRBuilder<> tmpBuilder(
        &function->getEntryBlock(),
        function->getEntryBlock().begin()
    );

    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

std::string Context::render() {
    std::string out;
    llvm::raw_string_ostream os(out);
    module->print(os, nullptr);
    return out;
}

void Context::bind(const std::string& name, Value* val) {
    scopes.back()[name] = val;
}

Value* Context::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end())
        return found->second;
    }
    return nullptr;
}

void Context::pop_scope() {
    scopes.pop_back();
}

void Context::push_scope() {
    scopes.emplace_back();
}
