#include <llvm/IR/Type.h>
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

void Context::emit(std::string line) {
    emission.push_back(line);
}

std::string Context::render() {
    std::string out;

    out += "i32 @main() {\n";
    out += "entry: \n";

    for (const auto& line : emission)
        out += line + "\n";

    out += "ret i32 0\n";
    out += "}\n";

    return out;
}
