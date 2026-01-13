#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

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
